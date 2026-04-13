import asyncio
import json
import random
from dataclasses import dataclass, field
from typing import Any

HOST = "0.0.0.0"
PORT = 5039

COMMAND_NAMES = {
    0: "none",
    1: "pass_ticks_after",
    2: "set_camera_position",
    3: "set_mouse_position",
    4: "set_mouse_left_button",
    5: "set_mouse_right_button",
    6: "add_mouse_scroll",
    7: "screenshot",
    8: "hud_info",
    9: "debug_command",
    10: "shutdown"
}

LAST_SESSION_TAG = random.randint(10, 10000) * 10000


@dataclass
class ClientConnection:
    reader: asyncio.StreamReader
    writer: asyncio.StreamWriter
    address: str


@dataclass
class ServerState:
    clients: list[ClientConnection] = field(default_factory=list)
    session_tag: int = 0
    net_id: int = 0

    def next_net_id(self) -> int:
        nid = self.net_id
        self.net_id += 1
        return nid

def resize_list(arr, n, fill_value=None):
    return arr[:n] + [fill_value] * (n - len(arr))


def make_command(command_type: str, enabled: bool, value: list[float]) -> dict[str, Any]:
    value = resize_list(value, 4, 0)
    return {"command_type": command_type, "enabled": enabled, "value": value}


def make_command_message(state: ServerState, commands: list[dict[str, Any]]) -> bytes:
    msg = {
        "net_session_tag": state.session_tag,
        "net_id": state.next_net_id(),
        "command_map": commands,
    }
    return json.dumps(msg, separators=(",", ":")).encode() + b"\n"


async def read_answers(client: ClientConnection, state: ServerState) -> None:
    buffer = b""
    try:
        while True:
            data = await client.reader.read(4096)
            if not data:
                break
            buffer += data
            while b"\n" in buffer:
                line, buffer = buffer.split(b"\n", 1)
                if not line:
                    continue
                try:
                    answer = json.loads(line)
                except json.JSONDecodeError as e:
                    print(f"[{client.address}] malformed JSON: {e}")
                    continue
                print(f"[{client.address}] answer: {answer}")
    except (ConnectionResetError, BrokenPipeError, OSError) as e:
        print(f"[{client.address}] read error: {e}")


async def handle_client(
    reader: asyncio.StreamReader,
    writer: asyncio.StreamWriter,
    state: ServerState,
) -> None:
    peername = writer.get_extra_info("peername")
    address = f"{peername[0]}:{peername[1]}" if peername else "unknown"
    client = ClientConnection(reader=reader, writer=writer, address=address)

    state.clients.append(client)
    print(f"[{address}] connected ({len(state.clients)} client(s))")

    try:
        await read_answers(client, state)
    finally:
        state.clients.remove(client)
        print(f"[{address}] disconnected ({len(state.clients)} client(s))")
        writer.close()
        try:
            await writer.wait_closed()
        except OSError:
            pass


async def console_loop(state: ServerState) -> None:
    loop = asyncio.get_event_loop()

    def read_line(text: str) -> str:
        return input(text)

    print("Commands:")
    for idx, name in COMMAND_NAMES.items():
        print(f"  {idx}: {name}")
    print("Enter '<num> [f1 f2 ...]' to queue a command, 0 to send.\n")

    global LAST_SESSION_TAG
    LAST_SESSION_TAG += 1
    state.session_tag = LAST_SESSION_TAG
    state.net_id = 0

    while True:
        pending: list[dict[str, Any]] = []

        while True:
            try:
                raw = await loop.run_in_executor(None, read_line, "cmd> ")
            except EOFError:
                return

            parts = raw.split()
            if not parts:
                continue

            try:
                num = int(parts[0])
            except ValueError:
                print("  expected a command number")
                continue

            if num == 0:
                break

            if num not in COMMAND_NAMES:
                print(f"  unknown command {num}, valid: 1-{max(COMMAND_NAMES)}")
                continue

            values: list[float] = []
            for token in parts[1:]:
                try:
                    values.append(float(token))
                except ValueError:
                    print(f"  skipping non-float '{token}'")

            pending = list(filter(lambda x: COMMAND_NAMES[num] != x["command_type"], pending))

            pending.append(make_command(COMMAND_NAMES[num], True, values))
            print(f"  queued: {COMMAND_NAMES[num]} {values}")

        if not pending:
            print("  nothing queued")
            continue

        if not state.clients:
            print("  no clients connected")
            continue

        target = state.clients[0]
        payload = make_command_message(state, pending)

        try:
            target.writer.write(payload)
            await target.writer.drain()
            print(f"[{target.address}] sent {len(pending)} command(s) (session={state.session_tag})")
        except (ConnectionResetError, BrokenPipeError, OSError) as e:
            print(f"[{target.address}] send error: {e}")


async def main() -> None:
    state = ServerState()

    async def on_connect(reader: asyncio.StreamReader, writer: asyncio.StreamWriter) -> None:
        await handle_client(reader, writer, state)

    server = await asyncio.start_server(on_connect, HOST, PORT)
    addrs = ", ".join(str(s.getsockname()) for s in server.sockets)
    print(f"listening on {addrs}")

    command_task = asyncio.create_task(console_loop(state))

    try:
        async with server:
            await server.serve_forever()
    finally:
        command_task.cancel()
        try:
            await command_task
        except asyncio.CancelledError:
            pass


if __name__ == "__main__":
    asyncio.run(main())
