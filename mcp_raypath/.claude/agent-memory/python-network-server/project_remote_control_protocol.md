---
name: Remote Control Protocol
description: TCP JSON protocol for remote_control_client — newline-delimited JSON, command/answer message structure, command name index
type: project
---

The remote control uses TCP with newline-delimited JSON (`\n` separator). The server is the controller; clients connect to it.

**Command (server to client):** has `net_session_tag`, `net_id`, and `command_map` (array of `{command_type, enabled, value}`).

**Answer (client to server):** has `net_session_tag`, `net_id`, and `answer_map` (array of `{command_type, enabled, verbal_message}`).

**Command name index:** 0=pass_ticks_after, 1=set_camera_position, 2=set_mouse_position, 3=set_mouse_left_button, 4=set_mouse_right_button, 5=add_mouse_scroll, 6=screenshot, 7=hud_info, 8=debug_command.

**Why:** The C++ Vulkan renderer acts as a TCP client connecting to this Python server for remote control (camera, input simulation, screenshots, HUD).

**How to apply:** All networking code in mcp_raypath must use this protocol. Messages must be `\n`-terminated JSON with compact separators. The server manages session_tag (increments per batch) and net_id (increments per message).
