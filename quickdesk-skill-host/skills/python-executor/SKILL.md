---
name: python-executor
description: Execute Python code and scripts on the remote host machine.
metadata:
  openclaw:
    os: ["win32", "darwin", "linux"]
    install:
      - id: binary
        kind: binary
        package: "python-executor"
---

# python-executor

This skill provides Python code execution on the remote host:

- `run_python` — execute inline Python code, returns stdout, stderr, and exit code
- `run_script` — execute a Python script file with optional arguments
- `get_python_info` — get Python version and environment information
- `install_package` — install a Python package via pip

## Usage

The agent starts a Rust MCP server that executes Python code via `python`
command (Windows) or `python3` (Unix). Supports configurable timeout,
working directory, and environment variables.

## Security

By default, dangerous operations like `subprocess`, `os.system`, `eval`,
`exec`, and file operations are blocked in inline execution mode.
For full Python access, use `run_script` with trusted script files.

## Examples

```
# Run inline code
run_python(code="print(2 + 2)")

# Run script file
run_script(path="analyze.py", args=["--input", "data.csv"])

# Get Python info
get_python_info()

# Install package
install_package(package="pandas")
```
