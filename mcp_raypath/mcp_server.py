
from __future__ import annotations

import asyncio
import logging
import sys
from pathlib import Path

from fastmcp import FastMCP
from fastmcp.utilities.types import Image
from fastmcp.tools import tool

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

    # to run mcp inspector:
    # npx @modelcontextprotocol/inspector python main.py
    # claude mcp add mcp_raycast --transport stdio -- python {project_root}\\mcp_raypath\\main.py

class RaypathMCPServer:
    """ MCP server exposing raypath tools.
        The testing api consists of 2 components:
        application launch and frame_command dealing.

        Interacting with the server should be done exclusively.

        Application build and launching, then it must be shut down manually.

        Remote_control is done by frame_command queue.
        Each frame_command has set of fcommand that will be executed in a same frame.
        After frame_command is commited it putted in a queue and will be executed in a single frame.
        After execution, the application sends answers back and it can be harvested.

        Basic usage is:
        --
        (message_launch, success_launch) = blocking_build_and_launch_application()

        fcommand_A()
        fcommand_B()
        fcommand_C()
        netId_1 = fcommand_commit_frame()
        fcommand_C()
        fcommand_D()
        fcommand_A()
        netId_2 = fcommand_commit_frame()
        fcommand_M()
        fcommand_L()
        fcommand_K()
        netId_3 = fcommand_commit_frame()

        (res_netId_1, responce_1, next_existed_1) = blocking_wait_next_frame_command_response();
        (res_netId_2, responce_2, next_existed_2) = blocking_wait_next_frame_command_response();
        (res_netId_3, responce_3, next_existed_3) = blocking_wait_next_frame_command_response(); # this will fully complete the last frame_command before shutdown

        # next_existed_1 == true, next_existed_2 == true, next_existed_3 == false, so you can iterate until commits out
        # res_netId_1 == netId_1, res_netId_2 == netId_2, res_netId_3 == netId_3, so you can confirm a response source
        # message_launch to use if anything with the build
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
        self.mcp.tool() (self.console_print)
        self.mcp.tool() (self.magic_formula)

        self.mcp.tool() (self.blocking_build_application)
        self.mcp.tool() (self.blocking_full_rebuild_application)
        self.mcp.tool() (self.blocking_build_and_launch_application)
        self.mcp.tool() (self.blocking_shutdown_application)
        self.mcp.tool() (self.get_session_id)
        self.mcp.tool() (self.blocking_wait_next_frame_command_response)
        self.mcp.tool() (self.fcommand_commit_frame)
        self.mcp.tool() (self.fcommand_pass_ticks_after)
        self.mcp.tool() (self.fcommand_set_camera_position)
        self.mcp.tool() (self.fcommand_set_mouse_position)
        self.mcp.tool() (self.fcommand_set_mouse_left_button)
        self.mcp.tool() (self.fcommand_set_mouse_right_button)
        self.mcp.tool() (self.fcommand_add_mouse_scroll)
        self.mcp.tool() (self.fcommand_screenshot)
        self.mcp.tool() (self.fcommand_hud_info)
        self.mcp.tool() (self.fcommand_debug_command)
        self.mcp.tool() (self.fcommand_session_log_rename)

    def _register_resources(self) -> None:

        @self.mcp.resource(
            uri="app://mcp_logs/screenshot/session_{session_id}/{net_id}_frameX.png",
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
            uri="app://mcp_logs/session_{session_id}_netY.log-last{last_lines}",
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

    @tool(
        tags={"test"},
        annotations={
            "readOnlyHint": True,
            "destructiveHint": False
        }
    )
    def console_print(self, text: str) -> str:
        """ Print something to server console."""
        self.log.info("[mcp print] %s", text)
        return f"printed: {text}"

    @tool(
        tags={"test"},
        annotations={
            "readOnlyHint": True,
            "destructiveHint": False
        }
    )
    def magic_formula(self, a: float, b: float) -> float:
        """ Test magic formula."""
        return a * b / a + b

    async def run(self) -> None:
        await self.mcp.run_stdio_async()

    @tool(
        tags={"app", "blocking"},
        annotations={
            "readOnlyHint": False,
            "destructiveHint": False
        }
    )
    def blocking_build_application(self, timeout_sec: int = 20)-> (str, bool):
        """ Usual build.
            Return: (message, success) """
        pass

    @tool(
        tags={"app", "blocking"},
        annotations={
            "readOnlyHint": False,
            "destructiveHint": False
        }
    )
    def blocking_full_rebuild_application(self, timeout_sec: int = 20) -> (str, bool):
        """ Full clean and rebulid.
            Return: (message, success) """
        pass

    @tool(
        tags={"app", "blocking"},
        annotations={
            "readOnlyHint": False,
            "destructiveHint": False
        }
    )
    def blocking_build_and_launch_application(self, timeout_sec: int = 20) -> (str, bool):
        """ The usual way of launch.
            Will be shutdown automatically if launch_application, or build_application called.
            Will be built automatically.
            session_id of the application. Will use it for logs and screenshot.
            get_session_id is now valid
            Return: (message, success)
            """
        pass

    @tool(
        tags={"app", "blocking"},
        annotations={
            "readOnlyHint": False,
            "destructiveHint": False
        }
    )
    def blocking_shutdown_application(self, timeout_sec: int = 10) -> None:
        """ Shutdown application."""

        # _fcommand_shutdown()
        # fcommand_commit_frame()

        pass

    @tool(
        tags={"app"},
        annotations={
            "readOnlyHint": True,
            "destructiveHint": False
        }
    )
    def get_session_id(self) -> int:
        """ Will use it for logs and screenshot."""
        pass

    @tool(
        tags={"remote_control", "blocking"},
        annotations={
            "readOnlyHint": False,
            "destructiveHint": False
        }
    )
    def blocking_wait_next_frame_command_response(self, timeout_sec: int = 10) -> (int, str, bool):
        """ Will block and wait until frame_command_net_id is received.
            return: (netId, JSON frame answer, is remaining waiting commands (need to launch get response one more time)) """
        pass

    @tool(
        tags={"remote_control"},
        annotations={
            "readOnlyHint": False,
            "destructiveHint": False
        }
    )
    def fcommand_commit_frame(self) -> int:
        """ After this fcommand the command_frame will be saved and executed.
            frame_commands are filling in a single request until commit.
            Next command frame will be executed in queue.
            return: netId of command frame"""
        pass

    @tool(
        tags={"remote_control"},
        annotations={
            "readOnlyHint": False,
            "destructiveHint": False
        }
    )
    def fcommand_pass_ticks_after(self, ticks_execute_after: int) -> None:
        """ After main command frame executed ticks_execute_after additional frames will be played.
            This makes sense only in -tickless mode"""
        pass

    @tool(
        tags={"remote_control"},
        annotations={
            "readOnlyHint": False,
            "destructiveHint": False
        }
    )
    def fcommand_set_camera_position(self, x_world_position_px: float, y_world_position_px: float, zoom_position: float) -> None:
        """ Set the center of the camera in a world space.
            zoom_position - Bigger number = bigger zoom in. """
        pass

    @tool(
        tags={"remote_control"},
        annotations={
            "readOnlyHint": False,
            "destructiveHint": False
        }
    )
    def fcommand_set_mouse_position(self, x_world_position_px: float, y_world_position_px: float) -> None:
        """ Moves the cursor.
            render_server.scene.show_cursor if true then visible.
            It's a rectangle with a border and pivot at top-left.
            On mouse button press it changes fill color.
            color config at the start of the application log."""
        pass

    @tool(
        tags={"remote_control"},
        annotations={
            "readOnlyHint": False,
            "destructiveHint": False
        }
    )
    def fcommand_set_mouse_left_button(self, new_button_state: bool) -> None:
        """ Setup mouse left button to the new_button_state. """
        pass

    @tool(
        tags={"remote_control"},
        annotations={
            "readOnlyHint": False,
            "destructiveHint": False
        }
    )
    def fcommand_set_mouse_right_button(self, new_button_state: bool) -> None:
        """ Setup mouse right button to the new_button_state. """
        pass

    @tool(
        tags={"remote_control"},
        annotations={
            "readOnlyHint": False,
            "destructiveHint": False
        }
    )
    def fcommand_add_mouse_scroll(self, add_scroll_delta: float) -> None:
        """ Add hardware input level scroll of mouse. """
        pass

    @tool(
        tags={"remote_control"},
        annotations={
            "readOnlyHint": False,
            "destructiveHint": False
        }
    )
    def fcommand_screenshot(self, disable_compress: bool = False) -> None:
        """ Create screenshot after the frame_command apply and save it as .png in {project_root}/mcp_logs/screenshot/.. folder.
          disable_compress: if more precise image required.
          In response, it will return screenshot path."""
        pass

    @tool(
        tags={"remote_control"},
        annotations={
            "readOnlyHint": False,
            "destructiveHint": False
        }
    )
    def fcommand_hud_info(self) -> None:
        """ Get overall frame info. """
        pass

    @tool(
        tags={"remote_control"},
        annotations={
            "readOnlyHint": False,
            "destructiveHint": False
        }
    )
    def fcommand_debug_command(self, x: float, y: float, z: float, w: float) -> None:
        """ Reserved command for development.
            just log info log by default. """
        pass

    @tool(
        tags={"remote_control"},
        annotations={
            "readOnlyHint": False,
            "destructiveHint": False
        }
    )
    def fcommand_session_log_rename(self) -> None:
        """ Redirects log name into session number format.
            Need for saving logs between sessions and bake-in log names in prompts.
            By default log name is mcp_logs/default.log.
            In response, it will return new log name."""
        pass

    def  _fcommand_shutdown(self):
        pass


def mcp_blocking_run() -> None:
    asyncio.run(RaypathMCPServer().run())
