# Remote Control Protocol

## Transport

- TCP, newline-delimited JSON lines (`\n` separator)
- Client connects to server at `host:port`
- Auto-reconnect on disconnect with ~500-2000ms backoff

## Message Types

### Command (server -> client)


```json
{
  "net_session_tag" : 431, "net_id" : 54, "command_map" : [
  { "command_type" : "pass_ticks_after", "enabled": true, "value" : [3.0, 0.0, 0.0, 0.0] },
  { "command_type" : "set_camera_position", "enabled": true, "value" : [3.0, 0.0, 0.0, 0.0] },
  { "command_type" : "hud_info", "enabled": true, "value" : [0.0, 0.0, 0.0, 0.0] } ]
}
```

Order is by tcp income.

### Answer (client -> server)

```json
{
  "net_session_tag" : 431, "net_id" : 54, "answer_map" : [
  { "command_type" : "pass_ticks_after", "enabled": true, "verbal_message" : "hello pass" },
  { "command_type" : "set_camera_position", "enabled": true, "verbal_message" : "hi ok" },
  { "command_type" : "hud_info", "enabled": true, "verbal_message" : "new other ok" } ]
}
```

## Command Name Index

actual at: project_root/scr/network/remote_control.h in remote_command_type enum. code copy:

    enum class remote_command_type {
        none = 0,  
        pass_ticks_after = 1, // [tick_amount] execute after command
        set_camera_position = 2, // [x, y, zoom] in pixel, world
        set_mouse_position = 3, // [x, y] in pixel, world
        set_mouse_left_button = 4, // [x > 0] bool
        set_mouse_right_button = 5, // [x > 0] bool
        add_mouse_scroll = 6, // [scalar]
        screenshot = 7, // [disable_compress > 0], screenshot of the last available frame, not frame of command
        hud_info = 8, // nothing
        debug_command = 9, // [x, y, z, w] args, could be some debug code, just log info log by default
        shutdown = 10,
        session_log_rename = 11, // rename default.log to session_3129_net120.log
        count
    };
