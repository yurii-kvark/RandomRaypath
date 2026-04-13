TODO:

* visualize cursor. outer rect: outer_rect_color=red. content_color_idle/left/right_pressed= yellow, green, purple. Describe cursor.
	* show_cursor = false # need for debug, in screen space
	* cursor_size_px = 10, 10
	* cursor_border_size_px = 2
	* cursor_border_color = red #
	* cursor_idle/left/right/both_pressed_color = red/green/yellow/purple

use cpp-generalist
create logical_system of logical_cursor_visualize.
add logical_cursor_visualize into base scene and setup it.

in configs render_server add: scene.show_cursor = bool, scene.cursor_size_px = (float, float), scene.cursor_border_size_px = float, visual_style.cursor_border_color = (color), visual_style.cursor_idle/left/right/both_pressed_color.

cursor is 2 overlayed rectangle. outer is border background is constant visual_style.cursor_border_color. inside color is dependent from mouse button state.




## Agentic auto coding integration

#### Claude Agentic integration overview: ![[agentic_claude_code_engine_loop.svg|697]]
### Parallel work design

Atomic single-user actions:
* Code change
* Compilation
* Execution

Data race table:
Code change - Compilation: MCP has a flag is_source_unlocked to check is agent can change the sources right now.
Compilation - Execution: resolved on MCP server side, waiting inside.

Agent Coder: Code change (only is_source_unlocked)
Agent Tester: Compilation, Execution (full MCP Server access)

### Custom Python MCP Server:

Project compilation, run and test is performing by MPC server.
#### Build:
* Build/full_rebuild in debug configuration
* return full error message
* is_source_unlocked - check is anyone do something on code.

#### Manage the process:
* Spawn with default kill timer. Return ID.
	* add headless, remote_control, tickless, tickless_delta_time, (optional) static_delta_time
* Check is alive by ID

#### Engine remote_control bridge:
* Access by process ID.

### Native integration:

* Print detailed logs in file

#### Remote control: 
* Has strict sequential order 
* Allow to perform N ticks with actual or T delta time.
* Creating screenshots with entry debug description
* add window input
* Close application

#### New launch flags
* **headless** - client working without window launching
* **remote_control** - allows MCP to control the client, do input, screenshot, call tick  etc.
* **tickless** - allowed to do tick only by external command.
* **tickless_delta_time** - in tickless mode will not include waiting for the command.
* **static_delta_time** - use external artificial delta time instead of real one.