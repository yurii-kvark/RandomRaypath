from __future__ import annotations

import asyncio
import json
import logging
import random
from dataclasses import dataclass

from ray_mcp import app_logging

app_logging.setup()

HOST = "0.0.0.0"
PORT = 5039

# Mirrors remote_command_type enum from the C++ application (remote_control.h).
COMMAND_TYPE: dict[int, str] = {
    0:  "none",
    1:  "pass_ticks_after",       # [tick_amount]
    2:  "set_camera_position",    # [x, y, zoom]  pixel / world space
    3:  "set_mouse_position",     # [x, y]        pixel / world space
    4:  "set_mouse_left_button",  # [x > 0] treated as bool
    5:  "set_mouse_right_button", # [x > 0] treated as bool
    6:  "add_mouse_scroll",       # [scalar]
    7:  "screenshot",             # [disable_compress > 0]
    8:  "hud_info",               # no args
    9:  "debug_command",          # [x, y, z, w]
    10: "shutdown",
    11: "session_log_rename",     # renames default.log → session_N_netM.log
}


@dataclass
class AnswerRecord:
    session_id: int  # net_session_tag from the app — identifies which session this answer belongs to
    net_id: int
    raw_json: str    # verbatim JSON string as received from the application


@dataclass
class _ClientConnection:
    reader: asyncio.StreamReader
    writer: asyncio.StreamWriter
    address: str


class RemoteControlServer:
    """Asyncio TCP server that drives the C++ raypath application via the
    remote_control protocol (newline-delimited JSON).

    The C++ app connects as a TCP client.  Commands are staged locally with
    add_command(), then sent as a single atomic frame_command_set on commit_set().
    Answers come back asynchronously and are queued for ordered harvesting via
    receive_next_answer().

    enable_console_debug: when True, an interactive console loop is co-launched
    alongside the server so you can stage and commit commands manually.
    """

    def __init__(
        self,
        host: str = HOST,
        port: int = PORT,
        enable_console_debug: bool = False,
    ) -> None:
        self._host = host
        self._port = port
        self.enable_console_debug = enable_console_debug

        # Session tag increments on each new client connection so the app can
        # discard messages that arrived from a previous session.
        self._session_tag: int = random.randint(10, 10_000) * 10_000
        self._net_id: int = 0

        self._staged: list[dict] = []   # uncommitted commands in the current set
        self._pending_count: int = 0    # committed sets not yet harvested

        self._client: _ClientConnection | None = None
        self._answer_queue: asyncio.Queue[AnswerRecord | None] = asyncio.Queue()
        self._server: asyncio.Server | None = None
        self._log = logging.getLogger("remote_control_server")

    # -------------------------------------------------------------------------
    # Lifecycle
    # -------------------------------------------------------------------------

    async def start(self) -> None:
        """Start listening. Runs until stop() is called or the task is cancelled."""
        self._server = await asyncio.start_server(
            self._on_client_connect, self._host, self._port
        )
        addrs = ", ".join(str(s.getsockname()) for s in self._server.sockets)
        self._log.info("listening on %s", addrs)

        tasks: list[asyncio.Task] = [
            asyncio.create_task(self._server.serve_forever()),
        ]
        if self.enable_console_debug:
            tasks.append(asyncio.create_task(self._console_loop()))

        try:
            await asyncio.gather(*tasks)
        finally:
            for t in tasks:
                t.cancel()

    async def stop(self) -> None:
        """Close the server socket gracefully."""
        if self._server:
            self._server.close()
            await self._server.wait_closed()
            self._server = None

    # -------------------------------------------------------------------------
    # Staging
    # -------------------------------------------------------------------------

    def add_command(self, command_type: str, value: list[float]) -> None:
        """Stage one command into the current uncommitted set.

        Each command_type occupies a single slot — staging the same type again
        replaces the previous entry, matching the C++ sparse-map semantics.
        The value list is zero-padded or truncated to exactly 4 floats.
        """
        padded: list[float] = (value + [0.0, 0.0, 0.0, 0.0])[:4]
        self._staged = [c for c in self._staged if c["command_type"] != command_type]
        self._staged.append({"command_type": command_type, "enabled": True, "value": padded})
        self._log.debug("staged %s %s  (total staged: %d)", command_type, padded, len(self._staged))

    def discard_staged(self) -> None:
        """Drop all staged commands without sending. Safe to call on an empty set."""
        n = len(self._staged)
        self._staged.clear()
        self._log.debug("discarded %d staged command(s)", n)

    def staged_count(self) -> int:
        """Number of frame_commands currently staged (not yet committed)."""
        return len(self._staged)

    # -------------------------------------------------------------------------
    # Commit
    # -------------------------------------------------------------------------

    async def commit_set(self) -> int:
        """Send the staged commands as a single frame_command_set.

        Clears the staging buffer and increments the internal pending counter.
        Returns the net_id that will appear in the matching AnswerRecord.
        Raises RuntimeError if no client is connected.
        Raises OSError on send failure.
        """
        if self._client is None:
            raise RuntimeError("commit_set: no client connected")

        net_id = self._net_id
        self._net_id += 1

        payload = (
            json.dumps(
                {
                    "net_session_tag": self._session_tag,
                    "net_id": net_id,
                    "command_map": list(self._staged),
                },
                separators=(",", ":"),
            ).encode()
            + b"\n"
        )

        self._client.writer.write(payload)
        await self._client.writer.drain()

        self._staged.clear()
        self._pending_count += 1
        self._log.debug(
            "committed set net_id=%d  session_tag=%d  pending=%d",
            net_id, self._session_tag, self._pending_count,
        )
        return net_id

    # -------------------------------------------------------------------------
    # Receive
    # -------------------------------------------------------------------------

    async def receive_next_answer(self, timeout_sec: float = 10.0) -> AnswerRecord | None:
        """Block until the next answer arrives from the application.

        Returns None on timeout or when the connection drops mid-wait.
        Check pending_answer_count() > 0 after this call to determine whether
        more answers are still expected (has_more semantics for the MCP layer).
        """
        try:
            record = await asyncio.wait_for(self._answer_queue.get(), timeout=timeout_sec)
        except asyncio.TimeoutError:
            self._log.warning("receive_next_answer: timed out after %.1fs", timeout_sec)
            return None

        if record is not None and self._pending_count > 0:
            self._pending_count -= 1
        return record   # None signals a disconnect

    def pending_answer_count(self) -> int:
        """Number of committed sets whose answers have not yet been harvested."""
        return self._pending_count

    def is_client_connected(self) -> bool:
        """True if the C++ application is currently connected."""
        return self._client is not None

    # -------------------------------------------------------------------------
    # Internal — connection handling
    # -------------------------------------------------------------------------

    async def _on_client_connect(
        self, reader: asyncio.StreamReader, writer: asyncio.StreamWriter
    ) -> None:
        peername = writer.get_extra_info("peername")
        address = f"{peername[0]}:{peername[1]}" if peername else "unknown"

        self._session_tag += 1
        self._net_id = 0
        self._client = _ClientConnection(reader=reader, writer=writer, address=address)
        self._log.info("client connected: %s  session_tag=%d", address, self._session_tag)

        try:
            await self._read_answers()
        finally:
            self._client = None
            self._log.info("client disconnected: %s", address)
            await self._answer_queue.put(None)  # unblock any waiting receive_next_answer
            writer.close()
            try:
                await writer.wait_closed()
            except OSError:
                pass

    async def _read_answers(self) -> None:
        assert self._client is not None
        buffer = b""
        try:
            while True:
                data = await self._client.reader.read(4096)
                if not data:
                    break
                buffer += data
                while b"\n" in buffer:
                    line, buffer = buffer.split(b"\n", 1)
                    if not line:
                        continue
                    try:
                        parsed = json.loads(line)
                    except json.JSONDecodeError as exc:
                        self._log.warning(
                            "malformed JSON from %s: %s", self._client.address, exc
                        )
                        continue
                    session_id = parsed.get("net_session_tag", -1)
                    net_id = parsed.get("net_id", -1)
                    self._log.debug("answer session_id=%d net_id=%d from %s, content: %s", session_id, net_id, self._client.address, parsed)
                    await self._answer_queue.put(
                        AnswerRecord(session_id=session_id, net_id=net_id, raw_json=json.dumps(parsed))
                    )
        except (ConnectionResetError, BrokenPipeError, OSError) as exc:
            self._log.error("read error from %s: %s", self._client.address, exc)

    # -------------------------------------------------------------------------
    # Internal — console debug loop
    # -------------------------------------------------------------------------

    async def _console_loop(self) -> None:
        loop = asyncio.get_event_loop()

        def _read_line(prompt: str) -> str:
            return input(prompt)

        print("RemoteControlServer debug console.")
        print("Commands:")
        for idx, name in sorted(COMMAND_TYPE.items()):
            print(f"  {idx:2d}: {name}")
        print("Enter '<num> [f1 f2 ...]' to stage.  '0' to send.  'q' to quit.\n")

        while True:
            try:
                raw = await loop.run_in_executor(None, _read_line, "cmd> ")
            except EOFError:
                break

            raw = raw.strip()

            if raw == "q":
                break

            parts = raw.split()
            if not parts:
                continue

            try:
                num = int(parts[0])
            except ValueError:
                print("  expected a command number")
                continue

            if num == 0:
                if self.staged_count() == 0:
                    print("  nothing staged")
                    continue
                if not self.is_client_connected():
                    print("  no client connected")
                    continue
                try:
                    net_id = await self.commit_set()
                    print(f"  sent net_id={net_id}  pending={self.pending_answer_count()}")
                except Exception as exc:
                    print(f"  send failed: {exc}")
                continue

            if num not in COMMAND_TYPE:
                print(f"  unknown: {num}  valid: 0-{max(COMMAND_TYPE)}")
                continue

            values: list[float] = []
            for token in parts[1:]:
                try:
                    values.append(float(token))
                except ValueError:
                    print(f"  skipping non-float '{token}'")

            self.add_command(COMMAND_TYPE[num], values)
            print(f"  staged: {COMMAND_TYPE[num]} {values}  (total: {self.staged_count()})")


def remote_control_run(
    host: str = HOST,
    port: int = PORT,
    enable_console_debug: bool = True,
) -> None:
    """Convenience entry point for standalone use."""
    asyncio.run(RemoteControlServer(host, port, enable_console_debug).start())
