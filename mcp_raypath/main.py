# from test_console_control import control_server_launch_blocking
#
# control_server_launch_blocking()
import asyncio
import os
import sys
import subprocess

#remote_control_server.remote_control_run()

############

from ray_mcp.mcp_server import mcp_blocking_run

mcp_blocking_run()
