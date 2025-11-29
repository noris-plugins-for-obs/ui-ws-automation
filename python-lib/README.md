# UI WS Automation Plugin Tool

This tool helps to test plugins on OBS Studio by delegating user-interface operation through obs-websocket.

- `obsws_ui.client.OBSWSUI` - A client class to communicate with the plugin.
- `obsws_ui.tool` - A CUI tool to help developing Python code using the library.

# Examples

Open the about dialog, take a screenshot, and close it.
```sh
obswsui-tool --trigger '[{}, {"objectName": "actionShowAbout"}]'
obswsui-tool --grab '[{"className": "OBSAbout"}]' OBSAbout.png
obswsui-tool --invoke-widget '[{"className": "OBSAbout"}]' close
```

## See also
- [obsws-python](https://github.com/aatikturk/obsws-python)
