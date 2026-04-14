// Copyright 2026 QuickDesk Authors
// python-executor — MCP server that executes Python code on the host.

use anyhow::Result;
use mcp_server_common::{McpServer, ToolDef, ToolHandler};
use serde_json::{json, Value};
use std::process::Stdio;
use std::path::Path;
use tokio::process::Command;

const DEFAULT_TIMEOUT_SECS: u64 = 60;
const MAX_OUTPUT_BYTES: usize = 512 * 1024; // 512 KB

struct PythonExecutorHandler;

impl ToolHandler for PythonExecutorHandler {
    fn server_name(&self) -> &str {
        "python-executor"
    }

    fn server_version(&self) -> &str {
        "0.1.0"
    }

    fn tool_defs(&self) -> Vec<ToolDef> {
        vec![
            ToolDef {
                name: "run_python",
                description: "Execute inline Python code on the remote host. Returns stdout, stderr, and exit code.",
                input_schema: json!({
                    "type": "object",
                    "properties": {
                        "code": {
                            "type": "string",
                            "description": "The Python code to execute"
                        },
                        "working_dir": {
                            "type": "string",
                            "description": "Working directory for the command. Default: system default"
                        },
                        "timeout_secs": {
                            "type": "integer",
                            "description": "Timeout in seconds. Default: 60"
                        }
                    },
                    "required": ["code"]
                }),
            },
            ToolDef {
                name: "run_script",
                description: "Execute a Python script file on the remote host with optional arguments.",
                input_schema: json!({
                    "type": "object",
                    "properties": {
                        "path": {
                            "type": "string",
                            "description": "Path to the Python script file (.py)"
                        },
                        "args": {
                            "type": "array",
                            "items": {"type": "string"},
                            "description": "Command line arguments for the script"
                        },
                        "working_dir": {
                            "type": "string",
                            "description": "Working directory for the script"
                        },
                        "timeout_secs": {
                            "type": "integer",
                            "description": "Timeout in seconds. Default: 120"
                        }
                    },
                    "required": ["path"]
                }),
            },
            ToolDef {
                name: "get_python_info",
                description: "Get Python version and environment information on the remote host.",
                input_schema: json!({
                    "type": "object",
                    "properties": {}
                }),
            },
            ToolDef {
                name: "install_package",
                description: "Install a Python package via pip on the remote host.",
                input_schema: json!({
                    "type": "object",
                    "properties": {
                        "package": {
                            "type": "string",
                            "description": "Package name or 'requirements.txt' path"
                        },
                        "timeout_secs": {
                            "type": "integer",
                            "description": "Timeout in seconds. Default: 300"
                        }
                    },
                    "required": ["package"]
                }),
            },
        ]
    }

    fn call_tool(
        &self,
        name: &str,
        args: Value,
    ) -> std::pin::Pin<Box<dyn std::future::Future<Output = Result<Value>> + Send + '_>> {
        let name = name.to_string();
        let args = args.clone();
        Box::pin(async move {
            match name.as_str() {
                "run_python" => run_python(args).await,
                "run_script" => run_script(args).await,
                "get_python_info" => get_python_info().await,
                "install_package" => install_package(args).await,
                _ => anyhow::bail!("unknown tool: {}", name),
            }
        })
    }
}

async fn run_python(args: Value) -> Result<Value> {
    let code = args
        .get("code")
        .and_then(|v| v.as_str())
        .ok_or_else(|| anyhow::anyhow!("missing required field: code"))?;

    let timeout_secs = args
        .get("timeout_secs")
        .and_then(|v| v.as_u64())
        .unwrap_or(DEFAULT_TIMEOUT_SECS);

    let working_dir = args.get("working_dir").and_then(|v| v.as_str());

    let mut cmd = if cfg!(target_os = "windows") {
        let mut c = Command::new("python");
        c.args(["-c", code]);
        c
    } else {
        // Try python3 first, fall back to python
        let mut c = Command::new("python3");
        c.args(["-c", code]);
        c
    };

    cmd.stdout(Stdio::piped());
    cmd.stderr(Stdio::piped());

    if let Some(dir) = working_dir {
        cmd.current_dir(dir);
    }

    execute_command(cmd, timeout_secs).await
}

async fn run_script(args: Value) -> Result<Value> {
    let path = args
        .get("path")
        .and_then(|v| v.as_str())
        .ok_or_else(|| anyhow::anyhow!("missing required field: path"))?;

    let args_list = args
        .get("args")
        .and_then(|v| v.as_array())
        .map(|arr| arr.iter().filter_map(|v| v.as_str()).collect::<Vec<_>>())
        .unwrap_or_default();

    let timeout_secs = args
        .get("timeout_secs")
        .and_then(|v| v.as_u64())
        .unwrap_or(120); // Longer timeout for scripts

    let working_dir = args.get("working_dir").and_then(|v| v.as_str());

    // Validate path
    let script_path = Path::new(path);
    if !script_path.exists() {
        return Ok(json!({
            "success": false,
            "error": format!("Script file not found: {}", path),
            "stdout": "",
            "stderr": "",
            "exit_code": -1
        }));
    }

    let mut cmd = if cfg!(target_os = "windows") {
        let mut c = Command::new("python");
        c.arg(path);
        c
    } else {
        let mut c = Command::new("python3");
        c.arg(path);
        c
    };

    for arg in args_list {
        cmd.arg(arg);
    }

    cmd.stdout(Stdio::piped());
    cmd.stderr(Stdio::piped());

    if let Some(dir) = working_dir {
        cmd.current_dir(dir);
    }

    execute_command(cmd, timeout_secs).await
}

async fn get_python_info() -> Result<Value> {
    let python_cmd = if cfg!(target_os = "windows") { "python" } else { "python3" };

    let mut cmd = Command::new(python_cmd);
    cmd.args(["-c", "import sys; import platform; import json; print(json.dumps({'version': sys.version, 'executable': sys.executable, 'platform': platform.platform(), 'cwd': __import__('os').getcwd()}))"]);

    cmd.stdout(Stdio::piped());
    cmd.stderr(Stdio::piped());

    let output = cmd.output().await?;

    if output.status.success() {
        let info = String::from_utf8_lossy(&output.stdout);
        if let Ok(info_json) = serde_json::from_str::<Value>(&info) {
            Ok(json!({
                "success": true,
                "python_version": info_json["version"],
                "python_executable": info_json["executable"],
                "platform": info_json["platform"],
                "cwd": info_json["cwd"]
            }))
        } else {
            Ok(json!({
                "success": true,
                "raw_output": info.to_string()
            }))
        }
    } else {
        Ok(json!({
            "success": false,
            "error": String::from_utf8_lossy(&output.stderr).to_string()
        }))
    }
}

async fn install_package(args: Value) -> Result<Value> {
    let package = args
        .get("package")
        .and_then(|v| v.as_str())
        .ok_or_else(|| anyhow::anyhow!("missing required field: package"))?;

    let timeout_secs = args
        .get("timeout_secs")
        .and_then(|v| v.as_u64())
        .unwrap_or(300); // Long timeout for pip install

    let mut cmd = if cfg!(target_os = "windows") {
        let mut c = Command::new("pip");
        c.args(["install", package]);
        c
    } else {
        let mut c = Command::new("pip3");
        c.args(["install", package]);
        c
    };

    cmd.stdout(Stdio::piped());
    cmd.stderr(Stdio::piped());

    execute_command(cmd, timeout_secs).await
}

async fn execute_command(mut cmd: Command, timeout_secs: u64) -> Result<Value> {
    let result = tokio::time::timeout(
        std::time::Duration::from_secs(timeout_secs),
        cmd.output(),
    )
    .await;

    match result {
        Ok(Ok(output)) => {
            let mut stdout = String::from_utf8_lossy(&output.stdout).to_string();
            let mut stderr = String::from_utf8_lossy(&output.stderr).to_string();

            let stdout_truncated = stdout.len() > MAX_OUTPUT_BYTES;
            let stderr_truncated = stderr.len() > MAX_OUTPUT_BYTES;
            if stdout_truncated {
                stdout.truncate(MAX_OUTPUT_BYTES);
                stdout.push_str("\n... [output truncated]");
            }
            if stderr_truncated {
                stderr.truncate(MAX_OUTPUT_BYTES);
                stderr.push_str("\n... [output truncated]");
            }

            Ok(json!({
                "success": output.status.success(),
                "exit_code": output.status.code(),
                "stdout": stdout,
                "stderr": stderr,
                "stdout_truncated": stdout_truncated,
                "stderr_truncated": stderr_truncated,
            }))
        }
        Ok(Err(e)) => {
            Ok(json!({
                "success": false,
                "error": format!("failed to execute: {}", e),
                "stdout": "",
                "stderr": "",
                "exit_code": -1
            }))
        }
        Err(_) => {
            Ok(json!({
                "success": false,
                "error": format!("command timed out after {} seconds", timeout_secs),
                "stdout": "",
                "stderr": "",
                "exit_code": -1
            }))
        }
    }
}

#[tokio::main]
async fn main() -> Result<()> {
    tracing_subscriber::fmt()
        .with_writer(std::io::stderr)
        .with_env_filter("python_executor=info")
        .init();

    let server = McpServer::new(PythonExecutorHandler);
    server.run().await
}
