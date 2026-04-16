
from __future__ import annotations

import asyncio
import logging
import os
import subprocess
import sys
from pathlib import Path
from typing import Literal, TypedDict

from ray_mcp import app_logging
from ray_mcp.remote_control_server import RemoteControlServer
from fastmcp import FastMCP
from fastmcp.utilities.types import Image

app_logging.setup()


class BuildResult(TypedDict):
    message: str
    success: bool


class FcommandResponse(TypedDict):
    net_id: int
    status: Literal["ok", "timeout", "disconnected"]
    answer: str   # JSON payload; empty string on non-ok status
    has_more: bool

class RaypathMCPServer:
    """ MCP server exposing raypath tools.
        The testing api consists of 2 components:
        application launch and frame_command_set dealing.

        Interacting with the server should be done exclusively.

        frame_command/fcommand is entry of frame_command_set map. Each command has a single slot.
        session_id - unique id of the launched control application
        net_id - unique id of the frame_command_set

        Application build and launching, then it must be shut down manually.

        Remote_control is done by frame_command_set queue.
        Each frame_command_set has fixed set of fcommand that will be executed in a same frame.
        After frame_command_set is commited it putted in a queue and will be executed in a single frame.
        After execution, the application sends answers back and it can be harvested.

        Basic usage is:
        --
        launch: BuildResult = blocking_build_and_launch_application()
        # launch["success"], launch["message"]

        fcommand_A()
        fcommand_B()
        fcommand_C()
        netId_1 = fcommand_commit_set()
        fcommand_C()
        fcommand_D()
        fcommand_A()
        netId_2 = fcommand_commit_set()
        fcommand_M()
        fcommand_L()
        fcommand_K()
        netId_3 = fcommand_commit_set()

        r1: FcommandResponse = blocking_wait_next_fcommand_response()
        r2: FcommandResponse = blocking_wait_next_fcommand_response()
        r3: FcommandResponse = blocking_wait_next_fcommand_response()  # completes last set before shutdown

        # r1["has_more"] == True, r2["has_more"] == True, r3["has_more"] == False  → iterate until has_more is False
        # r1["net_id"] == netId_1, ...                                              → confirm response source
        # r1["status"] == "ok" | "timeout" | "disconnected"                        → abort on non-ok
        # launch["message"] to use if anything with the build
        # get_session_id() to understand if application is still alive

        blocking_shutdown_application()
        --
    """

    def __init__(self, name: str = "mcp_raypath") -> None:
        self.log = logging.getLogger(name)
        self.mcp = FastMCP(name)
        self._remote = RemoteControlServer()
        self._app_process: subprocess.Popen | None = None
        self._project_root: Path = Path(__file__).resolve().parent.parent.parent
        self._register_tools()
        self._register_resources()
        self.log.info(f"[mcp print] _project_root {self._project_root}")
        self.log.info("[mcp print] launched RaypathMCPServer")

    def _register_tools(self) -> None:
        _ro = {"readOnlyHint": True,  "destructiveHint": False}
        _rw = {"readOnlyHint": False, "destructiveHint": False}

        self.mcp.tool(tags={"test"},                       annotations=_ro)(self.magic_print)
        self.mcp.tool(tags={"test"},                       annotations=_ro)(self.magic_formula)

        self.mcp.tool(tags={"app", "blocking"},            annotations=_rw)(self.blocking_build_application)
        self.mcp.tool(tags={"app", "blocking"},            annotations=_rw)(self.blocking_full_rebuild_application)
        self.mcp.tool(tags={"app", "blocking"},            annotations=_rw)(self.blocking_build_and_launch_application)
        self.mcp.tool(tags={"app", "blocking"},            annotations=_rw)(self.blocking_shutdown_application)
        self.mcp.tool(tags={"app"},                        annotations=_ro)(self.get_session_id)
        self.mcp.tool(tags={"app"},                        annotations=_ro)(self.is_application_running)
        self.mcp.tool(tags={"remote_control", "blocking"}, annotations=_rw)(self.blocking_wait_next_fcommand_response)
        self.mcp.tool(tags={"remote_control"},             annotations=_rw)(self.fcommand_commit_set)
        self.mcp.tool(tags={"remote_control"},             annotations=_rw)(self.fcommand_discard_frame_set)
        self.mcp.tool(tags={"remote_control"},             annotations=_ro)(self.fcommand_committed_queue_depth)
        self.mcp.tool(tags={"remote_control"},             annotations=_rw)(self.fcommand_pass_ticks_after)
        self.mcp.tool(tags={"remote_control"},             annotations=_rw)(self.fcommand_set_camera_position)
        self.mcp.tool(tags={"remote_control"},             annotations=_rw)(self.fcommand_set_mouse_position)
        self.mcp.tool(tags={"remote_control"},             annotations=_rw)(self.fcommand_set_mouse_left_button)
        self.mcp.tool(tags={"remote_control"},             annotations=_rw)(self.fcommand_set_mouse_right_button)
        self.mcp.tool(tags={"remote_control"},             annotations=_rw)(self.fcommand_add_mouse_scroll)
        self.mcp.tool(tags={"remote_control"},             annotations=_rw)(self.fcommand_screenshot)
        self.mcp.tool(tags={"remote_control"},             annotations=_rw)(self.fcommand_hud_info)
        self.mcp.tool(tags={"remote_control"},             annotations=_rw)(self.fcommand_debug_command)
        self.mcp.tool(tags={"remote_control"},             annotations=_rw)(self.fcommand_session_log_rename)

    def _register_resources(self) -> None:

        @self.mcp.resource(
            uri="app://mcp_logs/screenshot/session_{session_id}/{net_id}_frame_any.png",
            tags={"app"},
            mime_type="image/png", # mime types does not work in inspector
            annotations={
                "readOnlyHint": True
            }
        )
        async def get_screenshot_png(session_id: int, net_id: int) -> Image:
            """ Get PNG image of the frame screenshot. """
            pass

        @self.mcp.resource(
            uri="app://mcp_logs/session_{session_id}_net_any.log-last_{last_lines}",
            tags={"app"},
            mime_type="text/plain",
            annotations={
                "readOnlyHint": True
            }
        )
        async def get_log(session_id: int = -1, last_lines: int = -1) -> str:
            """
            :param session_id: log of the session or -1 to get the current one.
            :param last_lines: amount of last lines to send, -1 to send everything.
            :return: application log content
            """
            pass

        @self.mcp.resource(
            uri="app://config/current.toml",
            tags={"app"},
            mime_type="application/toml",
            annotations={
                "readOnlyHint": True
            }
        )
        async def get_config_toml() -> str:
            """ Get the current application config. """
            pass

    def magic_print(self, text: str) -> str:
        """ Print something to server console."""
        self.log.info("[mcp print] %s", text)
        return f"printed: {text}"

    def magic_formula(self, a: float, b: float) -> float:
        """ Test magic formula."""
        return a * b / (a + b)

    async def run(self) -> None:
        await asyncio.gather(
            self._remote.start(),
            self.mcp.run_stdio_async(),
        )

    async def _run_build(self, clean: bool, timeout_sec: int) -> BuildResult:

        if self._app_process is not None:
            try:
                self._app_process.kill()
                await asyncio.to_thread(self._app_process.wait)
            except OSError:
                pass
            self._app_process = None

        vulkan_sdk = os.environ.get("VULKAN_SDK", "")
        glslang = f"{vulkan_sdk}/Bin/glslangValidator.exe" if vulkan_sdk else "glslangValidator"
        build_dir = str(self._project_root / "cmake-build-debug")

        configure_cmd = [
            "cmake", "-B", build_dir, "-G", "Ninja",
            "-DRAY_DEBUG_NO_OPT=1",
            f"-DGLSLANG_VALIDATOR={glslang}",
        ]
        build_cmd = ["cmake", "--build", build_dir]
        if clean:
            build_cmd.append("--clean-first")

        cwd = str(self._project_root)
        log_path = self._project_root / "mcp_logs" / "mcp_build_log" / "build.log"
        log_path.parent.mkdir(parents=True, exist_ok=True)

        def _read_log() -> str:
            try:
                return log_path.read_text(encoding="utf-8")
            except OSError:
                return ""

        def _impl() -> BuildResult:
            try:
                with open(log_path, "w", encoding="utf-8") as lf:
                    r = subprocess.run(configure_cmd, cwd=cwd, stdout=lf, stderr=lf, timeout=timeout_sec)
                    if r.returncode != 0:
                        lf.flush()
                        return {"message": _read_log(), "success": False}
                    r = subprocess.run(build_cmd, cwd=cwd, stdout=lf, stderr=lf, timeout=timeout_sec)
                return {"message": _read_log(), "success": r.returncode == 0}
            except subprocess.TimeoutExpired:
                return {"message": "build timed out\n" + _read_log(), "success": False}
            except OSError as exc:
                return {"message": str(exc), "success": False}

        return await asyncio.to_thread(_impl)

    async def blocking_build_application(self, timeout_sec: int = 20) -> BuildResult:
        """ Usual build.
            Return: BuildResult with fields: message, success """
        # return await asyncio.to_thread(_impl, timeout_sec)
        return await self._run_build(clean=False, timeout_sec=timeout_sec)

    async def blocking_full_rebuild_application(self, timeout_sec: int = 20) -> BuildResult:
        """ Full clean and rebuild.
            Return: BuildResult with fields: message, success """
        # return await asyncio.to_thread(_impl, timeout_sec)
        return await self._run_build(clean=True, timeout_sec=timeout_sec)

    async def blocking_build_and_launch_application(self, timeout_sec: int = 20) -> BuildResult:
        """ The usual way of launch.
            Will be shutdown automatically if launch_application, or build_application called.
            Will be built automatically.
            session_id of the application. Will use it for logs and screenshot.
            get_session_id is now valid.
            Return: BuildResult with fields: message, success """

        result = await self._run_build(clean=False, timeout_sec=timeout_sec)
        if not result["success"]:
            return result

        exe_name = "RayClientRenderer.exe" if sys.platform == "win32" else "RayClientRenderer"
        exe = str(self._project_root / "cmake-build-debug" / exe_name)
        cwd = str(self._project_root)
        try:
            self._app_process = await asyncio.to_thread(
                lambda: subprocess.Popen([exe, "-config=mcp_control.toml"], cwd=cwd)
            )
        except Exception as exc:
            return {"message": f"launch failed: {exc}", "success": False}

        deadline = asyncio.get_event_loop().time() + timeout_sec
        while not self._remote.is_client_connected():
            if asyncio.get_event_loop().time() >= deadline:
                return {"message": "launched but app did not connect in time", "success": False}
            await asyncio.sleep(0.2)

        return {"message": "ok", "success": True}

    async def blocking_shutdown_application(self, timeout_sec: int = 10) -> None:
        """ Shutdown application.
            Shutdown is queued as a frame command — it executes after all previously
            committed frame_command_sets complete. Blocks until the app exits or
            timeout_sec elapses. """

        self._fcommand_shutdown()
        await self.fcommand_commit_set()

        if self._app_process is not None:
            def _wait() -> None:
                try:
                    self._app_process.wait(timeout=timeout_sec)
                except subprocess.TimeoutExpired:
                    self._app_process.kill()
            await asyncio.to_thread(_wait)
            self._app_process = None

    def get_session_id(self) -> int:
        """ Returns the current app_session_id. Used for log and screenshot resource URIs.
            Valid only after a successful blocking_build_and_launch_application call. """
        return self._remote.session_tag

    def is_application_running(self) -> bool:
        """ Returns True if the application process is currently running and connected. """
        return self._remote.is_client_connected()

    async def blocking_wait_next_fcommand_response(self, timeout_sec: int = 10) -> FcommandResponse:
        """ Wait for the next committed frame_command_set to be processed and return its answer.
            Call once per fcommand_commit_set, in commit order.
            Return: FcommandResponse with fields:
              net_id    — matches the netId returned by fcommand_commit_set
              status    — "ok" | "timeout" | "disconnected"
              answer    — JSON payload from the app; empty string on non-ok status
              has_more  — True if further committed sets are still pending; iterate until False """
        record = await self._remote.receive_next_answer(timeout_sec)
        if record is not None:
            return FcommandResponse(
                net_id=record.net_id,
                status="ok",
                answer=record.raw_json,
                has_more=self._remote.pending_answer_count() > 0,
            )
        if self._remote.is_client_connected():
            return FcommandResponse(net_id=-1, status="timeout", answer="", has_more=False)
        return FcommandResponse(net_id=-1, status="disconnected", answer="", has_more=False)

    async def fcommand_commit_set(self) -> int:
        """ After this command the frame_command_set will be sent and executed in a single frame request.
            Next frame_command_set will be executed in queue.
            return: netId of command frame"""
        return await self._remote.commit_set()

    def fcommand_discard_frame_set(self) -> None:
        """ Drop all currently staged fcommands without sending.
            Use to recover from a mistake before calling fcommand_commit_set. """
        self._remote.discard_staged()

    def fcommand_committed_queue_depth(self) -> int:
        """ Returns the number of committed frame_command_sets that have not yet been
            harvested via blocking_wait_next_fcommand_response.
            Use to verify queue depth before committing more sets or before shutdown. """
        return self._remote.pending_answer_count()

    def fcommand_pass_ticks_after(self, ticks_execute_after: int) -> None:
        """ After main command frame executed ticks_execute_after additional frames will be played."""
        self._remote.add_command("pass_ticks_after", [float(ticks_execute_after)])

    def fcommand_set_camera_position(self, x_world_position_px: float, y_world_position_px: float, zoom_position: float) -> None:
        """ Set the center of the camera in a world space.
            zoom_position - Bigger number = bigger zoom in. """
        self._remote.add_command("set_camera_position", [x_world_position_px, y_world_position_px, zoom_position])

    def fcommand_set_mouse_position(self, x_world_position_px: float, y_world_position_px: float) -> None:
        """ Moves the cursor.
            render_server.scene.show_cursor if true then visible.
            It's a rectangle with a border and pivot at top-left.
            On mouse button press it changes fill color.
            color config at the start of the application log."""
        self._remote.add_command("set_mouse_position", [x_world_position_px, y_world_position_px])

    def fcommand_set_mouse_left_button(self, new_button_state: bool) -> None:
        """ Setup mouse left button to the new_button_state. """
        self._remote.add_command("set_mouse_left_button", [1.0 if new_button_state else 0.0])

    def fcommand_set_mouse_right_button(self, new_button_state: bool) -> None:
        """ Setup mouse right button to the new_button_state. """
        self._remote.add_command("set_mouse_right_button", [1.0 if new_button_state else 0.0])

    def fcommand_add_mouse_scroll(self, add_scroll_delta: float) -> None:
        """ Add hardware input level scroll of mouse. """
        self._remote.add_command("add_mouse_scroll", [add_scroll_delta])

    def fcommand_screenshot(self, disable_compress: bool = False) -> None:
        """ Create screenshot after the frame_command_set apply and save it as .png in {project_root}/mcp_logs/screenshot/.. folder.
          disable_compress: if more precise image required.
          In response, it will return screenshot path."""
        self._remote.add_command("screenshot", [1.0 if disable_compress else 0.0])

    def fcommand_hud_info(self) -> None:
        """ Get overall frame info. """
        self._remote.add_command("hud_info", [])

    def fcommand_debug_command(self, x: float, y: float, z: float, w: float) -> None:
        """ Reserved command for development.
            just log info log by default. """
        self._remote.add_command("debug_command", [x, y, z, w])

    def fcommand_session_log_rename(self) -> None:
        """ Redirects log name into session number format.
            Need for saving logs between sessions and bake-in log names in prompts.
            By default log name is mcp_logs/default.log.
            In response, it will return new log name."""
        self._remote.add_command("session_log_rename", [])

    def _fcommand_shutdown(self):
        self._remote.add_command("shutdown", [])


def mcp_blocking_run() -> None:
    asyncio.run(RaypathMCPServer().run())
