use rmcp::handler::server::router::prompt::PromptRouter;
use rmcp::handler::server::tool::ToolRouter;
use rmcp::handler::server::wrapper::Parameters;
use rmcp::model::{
    Annotated, ErrorData, GetPromptRequestParams, GetPromptResult, Implementation,
    ListPromptsResult, ListResourceTemplatesResult, ListResourcesResult, PaginatedRequestParams,
    PromptMessage, PromptMessageRole, RawResource, RawResourceTemplate, ReadResourceRequestParams,
    ReadResourceResult, ResourceContents, ServerCapabilities, ServerInfo,
};
use rmcp::service::{RequestContext, RoleServer};
use rmcp::{prompt, prompt_handler, prompt_router, tool, tool_handler, tool_router, ServerHandler};
use schemars::JsonSchema;
use serde::Deserialize;
use serde_json::json;

use crate::ws_client::WsClient;

fn make_resource(uri: &str, name: &str, description: &str) -> Annotated<RawResource> {
    Annotated {
        raw: RawResource {
            uri: uri.to_string(),
            name: name.to_string(),
            title: None,
            description: Some(description.to_string()),
            mime_type: Some("application/json".to_string()),
            size: None,
            icons: None,
            meta: None,
        },
        annotations: None,
    }
}

// ---- Parameter structs ----

#[derive(Deserialize, JsonSchema)]
struct ConnectionIdParam {
    /// Connection ID
    connection_id: String,
}

#[derive(Deserialize, JsonSchema)]
struct ConnectDeviceParam {
    /// 9-digit device ID of the remote host
    device_id: String,
    /// Access code of the remote host
    access_code: String,
    /// Signaling server URL (optional, uses default if empty)
    server_url: Option<String>,
    /// Whether to show the remote desktop viewer window in QuickDesk UI. Defaults to true so the user can observe AI operations. Set to false for background/batch automation.
    show_window: Option<bool>,
}

#[derive(Deserialize, JsonSchema)]
struct ScreenshotParam {
    /// Connection ID
    connection_id: String,
    /// Maximum width of the screenshot in pixels. Image will be scaled down proportionally if wider than this value. Use this to reduce data transfer size.
    max_width: Option<i32>,
    /// Image format: "jpeg" (default) or "png"
    format: Option<String>,
    /// JPEG quality 1-100 (default: 80)
    quality: Option<i32>,
}

#[derive(Deserialize, JsonSchema)]
struct MouseClickParam {
    /// Connection ID
    connection_id: String,
    /// X coordinate
    x: f64,
    /// Y coordinate
    y: f64,
    /// Mouse button: "left", "right", or "middle". Defaults to "left".
    button: Option<String>,
}

#[derive(Deserialize, JsonSchema)]
struct MousePositionParam {
    /// Connection ID
    connection_id: String,
    /// X coordinate
    x: f64,
    /// Y coordinate
    y: f64,
}

#[derive(Deserialize, JsonSchema)]
struct MouseScrollParam {
    /// Connection ID
    connection_id: String,
    /// X coordinate
    x: f64,
    /// Y coordinate
    y: f64,
    /// Horizontal scroll delta
    delta_x: Option<f64>,
    /// Vertical scroll delta (positive=up, negative=down)
    delta_y: Option<f64>,
}

#[derive(Deserialize, JsonSchema)]
struct KeyboardTypeParam {
    /// Connection ID
    connection_id: String,
    /// Text to type
    text: String,
}

#[derive(Deserialize, JsonSchema)]
struct KeyboardHotkeyParam {
    /// Connection ID
    connection_id: String,
    /// Key names to press together, e.g. ["ctrl","c"], ["win","r"], ["alt","f4"]
    keys: Vec<String>,
}

#[derive(Deserialize, JsonSchema)]
struct SetClipboardParam {
    /// Connection ID
    connection_id: String,
    /// Text to set in remote clipboard
    text: String,
}

#[derive(Deserialize, JsonSchema)]
struct MouseDragParam {
    /// Connection ID
    connection_id: String,
    /// Start X coordinate
    start_x: f64,
    /// Start Y coordinate
    start_y: f64,
    /// End X coordinate
    end_x: f64,
    /// End Y coordinate
    end_y: f64,
    /// Mouse button: "left" (default), "right", or "middle"
    button: Option<String>,
}

#[derive(Deserialize, JsonSchema)]
struct KeyParam {
    /// Connection ID
    connection_id: String,
    /// Key name, e.g. "shift", "ctrl", "a", "enter". Same key names as keyboard_hotkey.
    key: String,
}

#[derive(Deserialize, JsonSchema)]
struct FindAndClickArgs {
    /// Description of the UI element to find, e.g. "the Save button", "the search input field"
    element_description: String,
    /// Connection ID of the remote desktop
    connection_id: String,
}

#[derive(Deserialize, JsonSchema)]
struct RunCommandArgs {
    /// The command to run, e.g. "dir C:\\", "ipconfig /all"
    command: String,
    /// Connection ID of the remote desktop
    connection_id: String,
}

// ---- MCP Server ----

#[derive(Clone)]
pub struct QuickDeskMcpServer {
    tool_router: ToolRouter<Self>,
    prompt_router: PromptRouter<Self>,
    ws: WsClient,
}

impl QuickDeskMcpServer {
    pub fn new(ws: WsClient) -> Self {
        Self {
            tool_router: Self::tool_router(),
            prompt_router: Self::prompt_router(),
            ws,
        }
    }
}

#[tool_router]
impl QuickDeskMcpServer {
    #[tool(description = "Get local host device ID, access code, signaling state, and client count. Use this to get credentials for connecting to the current computer.")]
    async fn get_host_info(&self) -> String {
        match self.ws.request("getHostInfo", json!({})).await {
            Ok(v) => serde_json::to_string_pretty(&v).unwrap_or_default(),
            Err(e) => format!("Error: {e}"),
        }
    }

    #[tool(description = "List clients currently connected to this host machine.")]
    async fn get_host_clients(&self) -> String {
        match self.ws.request("getHostClients", json!({})).await {
            Ok(v) => serde_json::to_string_pretty(&v).unwrap_or_default(),
            Err(e) => format!("Error: {e}"),
        }
    }

    #[tool(description = "Get overall status including host process, client process, and signaling server state.")]
    async fn get_status(&self) -> String {
        match self.ws.request("getStatus", json!({})).await {
            Ok(v) => serde_json::to_string_pretty(&v).unwrap_or_default(),
            Err(e) => format!("Error: {e}"),
        }
    }

    #[tool(description = "Get signaling server connection status for both host and client.")]
    async fn get_signaling_status(&self) -> String {
        match self.ws.request("getSignalingStatus", json!({})).await {
            Ok(v) => serde_json::to_string_pretty(&v).unwrap_or_default(),
            Err(e) => format!("Error: {e}"),
        }
    }

    #[tool(description = "Refresh the local host access code.")]
    async fn refresh_access_code(&self) -> String {
        match self.ws.request("refreshAccessCode", json!({})).await {
            Ok(v) => serde_json::to_string_pretty(&v).unwrap_or_default(),
            Err(e) => format!("Error: {e}"),
        }
    }

    #[tool(description = "List all active remote desktop connections.")]
    async fn list_connections(&self) -> String {
        match self.ws.request("listConnections", json!({})).await {
            Ok(v) => serde_json::to_string_pretty(&v).unwrap_or_default(),
            Err(e) => format!("Error: {e}"),
        }
    }

    #[tool(description = "Get detailed info for a specific remote connection.")]
    async fn get_connection_info(&self, params: Parameters<ConnectionIdParam>) -> String {
        match self
            .ws
            .request("getConnectionInfo", json!({ "connectionId": params.0.connection_id }))
            .await
        {
            Ok(v) => serde_json::to_string_pretty(&v).unwrap_or_default(),
            Err(e) => format!("Error: {e}"),
        }
    }

    #[tool(description = "Connect to a remote device. Returns a connection ID. By default, a remote desktop viewer window is shown so the user can observe your operations. Set show_window=false for silent background automation. To control the current computer, first call get_host_info to get the device ID and access code, then pass them here.")]
    async fn connect_device(&self, params: Parameters<ConnectDeviceParam>) -> String {
        let p = params.0;
        let mut req = json!({
            "deviceId": p.device_id,
            "accessCode": p.access_code,
        });
        if let Some(url) = p.server_url {
            req["serverUrl"] = json!(url);
        }
        if let Some(show) = p.show_window {
            req["showWindow"] = json!(show);
        }
        match self.ws.request("connectToHost", req).await {
            Ok(v) => serde_json::to_string_pretty(&v).unwrap_or_default(),
            Err(e) => format!("Error: {e}"),
        }
    }

    #[tool(description = "Disconnect from a remote device.")]
    async fn disconnect_device(&self, params: Parameters<ConnectionIdParam>) -> String {
        match self
            .ws
            .request("disconnectFromHost", json!({ "connectionId": params.0.connection_id }))
            .await
        {
            Ok(v) => serde_json::to_string_pretty(&v).unwrap_or_default(),
            Err(e) => format!("Error: {e}"),
        }
    }

    #[tool(description = "Disconnect all remote connections.")]
    async fn disconnect_all(&self) -> String {
        match self.ws.request("disconnectAll", json!({})).await {
            Ok(v) => serde_json::to_string_pretty(&v).unwrap_or_default(),
            Err(e) => format!("Error: {e}"),
        }
    }

    #[tool(description = "Capture a screenshot of the remote desktop. Returns base64 JPEG image data with width and height. Use max_width to reduce image size for faster transfer.")]
    async fn screenshot(&self, params: Parameters<ScreenshotParam>) -> String {
        let p = params.0;
        let mut req = json!({
            "connectionId": p.connection_id,
            "format": p.format.unwrap_or_else(|| "jpeg".to_string()),
            "quality": p.quality.unwrap_or(80),
        });
        if let Some(mw) = p.max_width {
            req["maxWidth"] = json!(mw);
        }
        match self.ws.request("screenshot", req).await {
            Ok(v) => serde_json::to_string_pretty(&v).unwrap_or_default(),
            Err(e) => format!("Error: {e}"),
        }
    }

    #[tool(description = "Click at coordinates on the remote desktop.")]
    async fn mouse_click(&self, params: Parameters<MouseClickParam>) -> String {
        let p = params.0;
        match self
            .ws
            .request(
                "mouseClick",
                json!({
                    "connectionId": p.connection_id,
                    "x": p.x, "y": p.y,
                    "button": p.button.unwrap_or_else(|| "left".to_string()),
                }),
            )
            .await
        {
            Ok(_) => format!("Clicked at ({}, {})", p.x, p.y),
            Err(e) => format!("Error: {e}"),
        }
    }

    #[tool(description = "Double-click at coordinates on the remote desktop.")]
    async fn mouse_double_click(&self, params: Parameters<MousePositionParam>) -> String {
        let p = params.0;
        match self
            .ws
            .request(
                "mouseDoubleClick",
                json!({
                    "connectionId": p.connection_id,
                    "x": p.x, "y": p.y,
                }),
            )
            .await
        {
            Ok(_) => format!("Double-clicked at ({}, {})", p.x, p.y),
            Err(e) => format!("Error: {e}"),
        }
    }

    #[tool(description = "Move the mouse cursor to coordinates on the remote desktop.")]
    async fn mouse_move(&self, params: Parameters<MousePositionParam>) -> String {
        let p = params.0;
        match self
            .ws
            .request(
                "mouseMove",
                json!({
                    "connectionId": p.connection_id,
                    "x": p.x, "y": p.y,
                }),
            )
            .await
        {
            Ok(_) => format!("Moved to ({}, {})", p.x, p.y),
            Err(e) => format!("Error: {e}"),
        }
    }

    #[tool(description = "Scroll the mouse wheel on the remote desktop.")]
    async fn mouse_scroll(&self, params: Parameters<MouseScrollParam>) -> String {
        let p = params.0;
        match self
            .ws
            .request(
                "mouseScroll",
                json!({
                    "connectionId": p.connection_id,
                    "x": p.x, "y": p.y,
                    "deltaX": p.delta_x.unwrap_or(0.0),
                    "deltaY": p.delta_y.unwrap_or(0.0),
                }),
            )
            .await
        {
            Ok(_) => "Scrolled".to_string(),
            Err(e) => format!("Error: {e}"),
        }
    }

    #[tool(description = "Type text on the remote desktop. Uses clipboard paste for reliable unicode input.")]
    async fn keyboard_type(&self, params: Parameters<KeyboardTypeParam>) -> String {
        let p = params.0;
        match self
            .ws
            .request(
                "keyboardType",
                json!({
                    "connectionId": p.connection_id,
                    "text": p.text,
                }),
            )
            .await
        {
            Ok(_) => format!("Typed: {}", p.text),
            Err(e) => format!("Error: {e}"),
        }
    }

    #[tool(description = "Send a keyboard shortcut (hotkey) on the remote desktop. Keys are pressed in order and released in reverse. Examples: [\"ctrl\",\"c\"], [\"ctrl\",\"shift\",\"esc\"], [\"win\",\"r\"], [\"alt\",\"f4\"]")]
    async fn keyboard_hotkey(&self, params: Parameters<KeyboardHotkeyParam>) -> String {
        let p = params.0;
        match self
            .ws
            .request(
                "keyboardHotkey",
                json!({
                    "connectionId": p.connection_id,
                    "keys": p.keys,
                }),
            )
            .await
        {
            Ok(_) => format!("Hotkey sent: {}", p.keys.join("+")),
            Err(e) => format!("Error: {e}"),
        }
    }

    #[tool(description = "Get the last known clipboard content from the remote desktop. Clipboard is synced automatically when something is copied on the remote machine.")]
    async fn get_clipboard(&self, params: Parameters<ConnectionIdParam>) -> String {
        let p = params.0;
        match self
            .ws
            .request("getClipboard", json!({"connectionId": p.connection_id}))
            .await
        {
            Ok(v) => serde_json::to_string_pretty(&v).unwrap_or_default(),
            Err(e) => format!("Error: {e}"),
        }
    }

    #[tool(description = "Set remote clipboard content.")]
    async fn set_clipboard(&self, params: Parameters<SetClipboardParam>) -> String {
        let p = params.0;
        match self
            .ws
            .request(
                "setClipboard",
                json!({
                    "connectionId": p.connection_id,
                    "text": p.text,
                }),
            )
            .await
        {
            Ok(_) => "Clipboard set".to_string(),
            Err(e) => format!("Error: {e}"),
        }
    }

    #[tool(description = "Drag the mouse from one position to another on the remote desktop. Useful for drag-and-drop, text selection, resizing windows, and moving objects.")]
    async fn mouse_drag(&self, params: Parameters<MouseDragParam>) -> String {
        let p = params.0;
        match self
            .ws
            .request(
                "mouseDrag",
                json!({
                    "connectionId": p.connection_id,
                    "startX": p.start_x, "startY": p.start_y,
                    "endX": p.end_x, "endY": p.end_y,
                    "button": p.button.unwrap_or_else(|| "left".to_string()),
                }),
            )
            .await
        {
            Ok(_) => format!(
                "Dragged from ({}, {}) to ({}, {})",
                p.start_x, p.start_y, p.end_x, p.end_y
            ),
            Err(e) => format!("Error: {e}"),
        }
    }

    #[tool(description = "Press and hold a key on the remote desktop. The key stays pressed until explicitly released with key_release. Useful for modifier keys (shift, ctrl, alt) while performing mouse operations like Ctrl+click for multi-select.")]
    async fn key_press(&self, params: Parameters<KeyParam>) -> String {
        let p = params.0;
        match self
            .ws
            .request(
                "keyPress",
                json!({
                    "connectionId": p.connection_id,
                    "key": p.key,
                }),
            )
            .await
        {
            Ok(_) => format!("Key pressed: {}", p.key),
            Err(e) => format!("Error: {e}"),
        }
    }

    #[tool(description = "Release a previously pressed key on the remote desktop. Must be paired with a prior key_press call.")]
    async fn key_release(&self, params: Parameters<KeyParam>) -> String {
        let p = params.0;
        match self
            .ws
            .request(
                "keyRelease",
                json!({
                    "connectionId": p.connection_id,
                    "key": p.key,
                }),
            )
            .await
        {
            Ok(_) => format!("Key released: {}", p.key),
            Err(e) => format!("Error: {e}"),
        }
    }

    #[tool(description = "Get the remote screen resolution.")]
    async fn get_screen_size(&self, params: Parameters<ConnectionIdParam>) -> String {
        match self
            .ws
            .request("getScreenSize", json!({ "connectionId": params.0.connection_id }))
            .await
        {
            Ok(v) => serde_json::to_string_pretty(&v).unwrap_or_default(),
            Err(e) => format!("Error: {e}"),
        }
    }
}

#[prompt_router]
impl QuickDeskMcpServer {
    /// System prompt for AI agents operating a remote desktop via QuickDesk.
    /// Explains the screenshot-analyze-act loop, coordinate system, best practices, and available tools.
    #[prompt(name = "operate_remote_desktop")]
    async fn operate_remote_desktop(&self) -> Result<Vec<PromptMessage>, ErrorData> {
        Ok(vec![PromptMessage::new_text(
            PromptMessageRole::User,
            "You are controlling a remote desktop through QuickDesk MCP tools. Follow this workflow:\n\
            \n\
            ## Core Loop: Screenshot → Analyze → Act → Verify\n\
            \n\
            1. **Screenshot**: Call `screenshot` to see the current screen state. Use `max_width: 1280` to reduce transfer size while keeping enough detail.\n\
            2. **Analyze**: Study the screenshot to understand what's on screen. Identify UI elements, their positions, and the current state.\n\
            3. **Act**: Perform ONE action (click, type, hotkey, etc.) based on your analysis.\n\
            4. **Verify**: Take another screenshot to confirm the action succeeded. If it didn't, retry or try an alternative approach.\n\
            \n\
            ## Coordinate System\n\
            \n\
            - Call `get_screen_size` to get the remote desktop resolution (e.g. 1920x1080).\n\
            - Screenshot coordinates map directly to screen coordinates when using full resolution.\n\
            - If you used `max_width` in screenshot, scale coordinates proportionally: `actual_x = screenshot_x * (screen_width / image_width)`.\n\
            \n\
            ## Best Practices\n\
            \n\
            - **Wait after actions**: UI animations and loading take time. After clicking or typing, wait briefly before taking the next screenshot.\n\
            - **One action at a time**: Don't chain multiple actions without verifying each one.\n\
            - **Use keyboard shortcuts**: They are more reliable than clicking menus. E.g. Ctrl+S to save, Ctrl+C to copy, Win+R to open Run dialog.\n\
            - **Type via keyboard_type**: It uses clipboard paste internally for reliable unicode support.\n\
            - **Error recovery**: If an action fails, take a screenshot to understand the current state, then try an alternative approach.\n\
            - **Modifier+click**: Use `key_press` to hold a modifier (e.g. \"ctrl\"), then `mouse_click`, then `key_release` the modifier. This enables Ctrl+click for multi-select.\n\
            - **Drag operations**: Use `mouse_drag` for selecting text, moving files, resizing windows, or drag-and-drop.\n\
            \n\
            ## Available Tools Summary\n\
            \n\
            | Category | Tools |\n\
            |----------|-------|\n\
            | Connection | connect_device, disconnect_device, list_connections |\n\
            | Vision | screenshot, get_screen_size |\n\
            | Mouse | mouse_click, mouse_double_click, mouse_move, mouse_scroll, mouse_drag |\n\
            | Keyboard | keyboard_type, keyboard_hotkey, key_press, key_release |\n\
            | Clipboard | get_clipboard, set_clipboard |\n\
            | System | get_host_info, get_status |"
        )])
    }

    /// Guide for finding and clicking a specific UI element on the remote desktop.
    #[prompt(name = "find_and_click")]
    async fn find_and_click(
        &self,
        params: Parameters<FindAndClickArgs>,
    ) -> Result<Vec<PromptMessage>, ErrorData> {
        let args = params.0;
        Ok(vec![PromptMessage::new_text(
            PromptMessageRole::User,
            format!(
                "Find and click the following element on the remote desktop: \"{}\"\n\
                Connection ID: {}\n\
                \n\
                Follow these steps:\n\
                \n\
                1. Call `screenshot` with connection_id=\"{}\" and max_width=1280 to see the current screen.\n\
                2. Analyze the screenshot to locate \"{}\".\n\
                   - Look for text labels, icons, or visual cues that match the description.\n\
                   - If the element is not visible, you may need to scroll or navigate to find it.\n\
                3. Call `get_screen_size` to get the actual screen resolution.\n\
                4. Calculate the actual coordinates: if the screenshot is smaller than the screen, scale up proportionally.\n\
                   - `actual_x = screenshot_x * (screen_width / image_width)`\n\
                   - `actual_y = screenshot_y * (screen_height / image_height)`\n\
                5. Click at the calculated coordinates using `mouse_click`.\n\
                6. Take another screenshot to verify the click worked (e.g. a menu opened, a button was pressed, a page navigated).\n\
                7. If the click didn't hit the right target, adjust coordinates and retry.\n\
                \n\
                Tips:\n\
                - Click the CENTER of the element, not the edge.\n\
                - For small elements (checkboxes, radio buttons), be extra precise with coordinates.\n\
                - If the element is in a scrollable area and not visible, use mouse_scroll first.",
                args.element_description, args.connection_id,
                args.connection_id, args.element_description
            ),
        )])
    }

    /// Guide for running a command or script on the remote desktop via terminal/command prompt.
    #[prompt(name = "run_command")]
    async fn run_command(
        &self,
        params: Parameters<RunCommandArgs>,
    ) -> Result<Vec<PromptMessage>, ErrorData> {
        let args = params.0;
        Ok(vec![PromptMessage::new_text(
            PromptMessageRole::User,
            format!(
                "Run the following command on the remote desktop: `{}`\n\
                Connection ID: {}\n\
                \n\
                Follow these steps:\n\
                \n\
                1. Take a screenshot to see the current screen state.\n\
                2. Open a terminal / command prompt:\n\
                   - **Windows**: Use `keyboard_hotkey` with keys [\"win\", \"r\"], wait, then `keyboard_type` \"cmd\" and press Enter. Or use [\"win\", \"x\"] then \"i\" for PowerShell.\n\
                   - **macOS**: Use Spotlight with [\"meta\", \"space\"], type \"Terminal\", press Enter.\n\
                   - **Linux**: Try [\"ctrl\", \"alt\", \"t\"] to open terminal.\n\
                   - If a terminal is already open, skip this step.\n\
                3. Take a screenshot to verify the terminal is open and ready for input.\n\
                4. Type the command using `keyboard_type` with text=\"{}\".\n\
                5. Press Enter using `keyboard_hotkey` with keys [\"enter\"].\n\
                6. Wait briefly for the command to execute.\n\
                7. Take a screenshot to capture the output.\n\
                8. If the output is long, you may need to scroll up to see all of it.\n\
                \n\
                Tips:\n\
                - For long-running commands, take multiple screenshots to monitor progress.\n\
                - If the command requires elevated privileges, you may need to run as administrator.\n\
                - Use `get_clipboard` after selecting output text (Ctrl+A in terminal, then Ctrl+C) to get the text content.",
                args.command, args.connection_id, args.command
            ),
        )])
    }
}

#[tool_handler]
#[prompt_handler]
impl ServerHandler for QuickDeskMcpServer {
    fn get_info(&self) -> ServerInfo {
        ServerInfo {
            capabilities: ServerCapabilities::builder()
                .enable_tools()
                .enable_resources()
                .enable_prompts()
                .build(),
            server_info: Implementation {
                name: "quickdesk-mcp".to_string(),
                version: env!("CARGO_PKG_VERSION").to_string(),
                ..Default::default()
            },
            instructions: Some(
                "QuickDesk MCP Server - Control remote desktops via QuickDesk. \
                 Use get_host_info to get the local device credentials, then \
                 connect_device to establish a remote session. After connecting, \
                 use screenshot, mouse_click, keyboard_type, etc. to interact \
                 with the remote desktop. \
                 QuickDesk is a desktop application with a GUI. When you connect \
                 to a device, a remote desktop viewer window is shown by default \
                 so the user can observe your operations in real time. You can \
                 set show_window=false in connect_device for silent batch automation."
                    .to_string(),
            ),
            ..Default::default()
        }
    }

    async fn list_resources(
        &self,
        _request: Option<PaginatedRequestParams>,
        _context: RequestContext<RoleServer>,
    ) -> Result<ListResourcesResult, ErrorData> {
        let mut resources = vec![
            make_resource(
                "quickdesk://host",
                "Host Info",
                "Local device ID, access code, signaling state and connected client count",
            ),
            make_resource(
                "quickdesk://status",
                "System Status",
                "Overall status of host process, client process and signaling server",
            ),
        ];

        if let Ok(v) = self.ws.request("listConnections", json!({})).await {
            if let Some(conns) = v.get("connections").and_then(|c| c.as_array()) {
                for conn in conns {
                    let id = conn
                        .get("connectionId")
                        .and_then(|v| v.as_str())
                        .unwrap_or("unknown");
                    let device_id = conn
                        .get("deviceId")
                        .and_then(|v| v.as_str())
                        .unwrap_or("unknown");
                    resources.push(make_resource(
                        &format!("quickdesk://connection/{id}"),
                        &format!("Connection {id} (device {device_id})"),
                        &format!(
                            "Detailed info for remote connection {id} to device {device_id}"
                        ),
                    ));
                }
            }
        }

        Ok(ListResourcesResult {
            resources,
            next_cursor: None,
            meta: None,
        })
    }

    async fn list_resource_templates(
        &self,
        _request: Option<PaginatedRequestParams>,
        _context: RequestContext<RoleServer>,
    ) -> Result<ListResourceTemplatesResult, ErrorData> {
        Ok(ListResourceTemplatesResult {
            resource_templates: vec![Annotated {
                raw: RawResourceTemplate {
                    uri_template: "quickdesk://connection/{connectionId}".to_string(),
                    name: "Connection Info".to_string(),
                    title: None,
                    description: Some(
                        "Detailed info for a specific remote connection by connection ID"
                            .to_string(),
                    ),
                    mime_type: Some("application/json".to_string()),
                    icons: None,
                },
                annotations: None,
            }],
            next_cursor: None,
            meta: None,
        })
    }

    async fn read_resource(
        &self,
        request: ReadResourceRequestParams,
        _context: RequestContext<RoleServer>,
    ) -> Result<ReadResourceResult, ErrorData> {
        let uri = &request.uri;

        let result = if uri == "quickdesk://host" {
            self.ws.request("getHostInfo", json!({})).await
        } else if uri == "quickdesk://status" {
            self.ws.request("getStatus", json!({})).await
        } else if let Some(conn_id) = uri.strip_prefix("quickdesk://connection/") {
            self.ws
                .request("getConnectionInfo", json!({ "connectionId": conn_id }))
                .await
        } else {
            return Err(ErrorData::invalid_params(
                format!("Unknown resource URI: {uri}"),
                None,
            ));
        };

        match result {
            Ok(v) => {
                let text = serde_json::to_string_pretty(&v).unwrap_or_default();
                Ok(ReadResourceResult {
                    contents: vec![ResourceContents::TextResourceContents {
                        uri: uri.to_string(),
                        mime_type: Some("application/json".to_string()),
                        text,
                        meta: None,
                    }],
                })
            }
            Err(e) => Err(ErrorData::internal_error(e, None)),
        }
    }
}
