
from __future__ import annotations

import asyncio
import logging
import sys
from pathlib import Path

from typing import Literal, TypedDict

from fastmcp import FastMCP
from fastmcp.utilities.types import Image


class BuildResult(TypedDict):
    message: str
    success: bool


class FcommandResponse(TypedDict):
    net_id: int
    status: Literal["ok", "timeout", "disconnected"]
    answer: str   # JSON payload; empty string on non-ok status
    has_more: bool

log_dir = Path("logs")
log_dir.mkdir(exist_ok=True)

# todo: move logging in another file
logging.basicConfig(
    level=logging.DEBUG,
    format="%(asctime)s [%(levelname)s] %(name)s: %(message)s",
    handlers=[
        logging.StreamHandler(sys.stderr),
        logging.FileHandler(log_dir / "mcp_server.log", mode="a", encoding="utf-8")
    ]
)


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
        self._register_tools()
        self._register_resources()
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
        self.mcp.tool(tags={"remote_control"},             annotations=_ro)(self.fcommand_pending_frame_set_count)
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
        await self.mcp.run_stdio_async()

    async def blocking_build_application(self, timeout_sec: int = 20) -> BuildResult:
        """ Usual build.
            Return: BuildResult with fields: message, success """
        # return await asyncio.to_thread(_impl, timeout_sec)
        pass

    async def blocking_full_rebuild_application(self, timeout_sec: int = 20) -> BuildResult:
        """ Full clean and rebuild.
            Return: BuildResult with fields: message, success """
        # return await asyncio.to_thread(_impl, timeout_sec)
        pass

    async def blocking_build_and_launch_application(self, timeout_sec: int = 20) -> BuildResult:
        """ The usual way of launch.
            Will be shutdown automatically if launch_application, or build_application called.
            Will be built automatically.
            session_id of the application. Will use it for logs and screenshot.
            get_session_id is now valid.
            Return: BuildResult with fields: message, success """
        pass

    async def blocking_shutdown_application(self, timeout_sec: int = 10) -> None:
        """ Shutdown application.
            Shutdown is queued as a frame command — it executes after all previously
            committed frame_command_sets complete. Blocks until the app exits or
            timeout_sec elapses. """

        # _fcommand_shutdown()
        # fcommand_commit_set()
        # await asyncio.to_thread(_impl, timeout_sec)

        pass

    def get_session_id(self) -> int:
        """ Returns the current app_session_id. Used for log and screenshot resource URIs.
            Valid only after a successful blocking_build_and_launch_application call. """
        pass

    def is_application_running(self) -> bool:
        """ Returns True if the application process is currently running and connected. """
        pass

    async def blocking_wait_next_fcommand_response(self, timeout_sec: int = 10) -> FcommandResponse:
        """ Wait for the next committed frame_command_set to be processed and return its answer.
            Call once per fcommand_commit_set, in commit order.
            Return: FcommandResponse with fields:
              net_id    — matches the netId returned by fcommand_commit_set
              status    — "ok" | "timeout" | "disconnected"
              answer    — JSON payload from the app; empty string on non-ok status
              has_more  — True if further committed sets are still pending; iterate until False """
        # return await asyncio.to_thread(_impl, timeout_sec)
        pass

    def fcommand_commit_set(self) -> int:
        """ After this command the frame_command_set will be sent and executed in a single frame request.
            Next frame_command_set will be executed in queue.
            return: netId of command frame"""
        pass

    def fcommand_discard_frame_set(self) -> None:
        """ Drop all currently staged fcommands without sending.
            Use to recover from a mistake before calling fcommand_commit_set. """
        pass

    def fcommand_committed_queue_depth(self) -> int:
        """ Returns the number of committed frame_command_sets that have not yet been
            harvested via blocking_wait_next_fcommand_response.
            Use to verify queue depth before committing more sets or before shutdown. """
        pass

    def fcommand_pass_ticks_after(self, ticks_execute_after: int) -> None:
        """ After main command frame executed ticks_execute_after additional frames will be played."""
        pass

    def fcommand_set_camera_position(self, x_world_position_px: float, y_world_position_px: float, zoom_position: float) -> None:
        """ Set the center of the camera in a world space.
            zoom_position - Bigger number = bigger zoom in. """
        pass

    def fcommand_set_mouse_position(self, x_world_position_px: float, y_world_position_px: float) -> None:
        """ Moves the cursor.
            render_server.scene.show_cursor if true then visible.
            It's a rectangle with a border and pivot at top-left.
            On mouse button press it changes fill color.
            color config at the start of the application log."""
        pass

    def fcommand_set_mouse_left_button(self, new_button_state: bool) -> None:
        """ Setup mouse left button to the new_button_state. """
        pass

    def fcommand_set_mouse_right_button(self, new_button_state: bool) -> None:
        """ Setup mouse right button to the new_button_state. """
        pass

    def fcommand_add_mouse_scroll(self, add_scroll_delta: float) -> None:
        """ Add hardware input level scroll of mouse. """
        pass

    def fcommand_screenshot(self, disable_compress: bool = False) -> None:
        """ Create screenshot after the frame_command_set apply and save it as .png in {project_root}/mcp_logs/screenshot/.. folder.
          disable_compress: if more precise image required.
          In response, it will return screenshot path."""
        pass

    def fcommand_hud_info(self) -> None:
        """ Get overall frame info. """
        pass

    def fcommand_debug_command(self, x: float, y: float, z: float, w: float) -> None:
        """ Reserved command for development.
            just log info log by default. """
        pass

    def fcommand_session_log_rename(self) -> None:
        """ Redirects log name into session number format.
            Need for saving logs between sessions and bake-in log names in prompts.
            By default log name is mcp_logs/default.log.
            In response, it will return new log name."""
        pass

    def _fcommand_shutdown(self):
        pass


def mcp_blocking_run() -> None:
    asyncio.run(RaypathMCPServer().run())
