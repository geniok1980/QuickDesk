# QuickDesk MCP Configuration Examples

Sample configuration files for connecting AI agents to QuickDesk via MCP.

## Files

| File | Description |
|------|-------------|
| `cursor-mcp.json` | Cursor IDE — place as `.cursor/mcp.json` in your project root |
| `claude-desktop-config.json` | Claude Desktop (Windows) — place at `%APPDATA%\Claude\claude_desktop_config.json` |
| `claude-desktop-config-mac.json` | Claude Desktop (macOS) — place at `~/Library/Application Support/Claude/claude_desktop_config.json` |
| `custom-ws-url.json` | Connect to QuickDesk running on a different machine |
| `with-auth-token.json` | Use authentication token via environment variable |

## Usage

1. Copy the appropriate file to the location required by your AI client
2. Update the `command` path to point to your `quickdesk-mcp` binary
3. Make sure QuickDesk is running
4. Restart your AI client to pick up the configuration

See the full [MCP Integration Guide](../../docs/mcp-integration.md) for details.
