# UI WS Automation Plugin for OBS Studio

## Features

This plugin helps to test plugins on OBS Studio by delegating user-interface operation through obs-websocket.

These operations are available:
- Menu item: list items, trigger item.
- Dialog: list dialogs, ...

> [!WARNING]
> This plugin exposes internal UI inspection and controls of OBS Studio through obs-websocket.
> Use it with caution and only for testing purposes.
> Do *not* expose obs-websocket to untrusted clients or networks while using this plugin.

## Configuration

This plugin requires a configuration file to enable its features.
To enable the plugin, create a file `plugin_config/ui-ws-automation/enable.json` with the following content:
```json
{
  "enable": true
}
```
