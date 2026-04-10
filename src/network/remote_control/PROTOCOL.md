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

| Index | Name                    |
|-------|-------------------------|
| 0     | pass_ticks_after        |
| 1     | set_camera_position     |
| 2     | set_mouse_position      |
| 3     | set_mouse_left_button   |
| 4     | set_mouse_right_button  |
| 5     | add_mouse_scroll        |
| 6     | screenshot              |
| 7     | hud_info                |
| 8     | debug_command           |