#!/usr/bin/env python3
"""
QuickDesk Python Executor - Test Implementation
This is a standalone Python version for testing purposes.
The production version is in Rust at quickdesk-skill-host/skills/python-executor/
"""

import json
import subprocess
import sys
import os
import traceback
from pathlib import Path
from typing import Dict, Any, List, Optional

VERSION = "0.1.0"

class PythonExecutor:
    """Executes Python code and scripts on the remote machine."""
    
    def __init__(self):
        self.version = VERSION
    
    def run_python(
        self,
        code: str,
        timeout: int = 60,
        working_dir: Optional[str] = None
    ) -> Dict[str, Any]:
        """
        Execute inline Python code.
        
        Args:
            code: Python code to execute
            timeout: Execution timeout in seconds
            working_dir: Optional working directory
            
        Returns:
            Dict with stdout, stderr, exit_code
        """
        python_cmd = "python" if sys.platform == "win32" else "python3"
        
        try:
            result = subprocess.run(
                [python_cmd, "-c", code],
                capture_output=True,
                text=True,
                timeout=timeout,
                cwd=working_dir
            )
            return {
                "success": result.returncode == 0,
                "stdout": result.stdout,
                "stderr": result.stderr,
                "exit_code": result.returncode
            }
        except subprocess.TimeoutExpired:
            return {
                "success": False,
                "error": f"Timeout after {timeout} seconds",
                "stdout": "",
                "stderr": "",
                "exit_code": -1
            }
        except Exception as e:
            return {
                "success": False,
                "error": str(e),
                "stdout": "",
                "stderr": traceback.format_exc(),
                "exit_code": -1
            }
    
    def run_script(
        self,
        path: str,
        args: List[str] = None,
        working_dir: Optional[str] = None,
        timeout: int = 120
    ) -> Dict[str, Any]:
        """
        Execute Python script file.
        
        Args:
            path: Path to .py file
            args: Command line arguments
            working_dir: Optional working directory
            timeout: Timeout in seconds
            
        Returns:
            Dict with stdout, stderr, exit_code
        """
        script_path = Path(path)
        if not script_path.exists():
            return {
                "success": False,
                "error": f"Script not found: {path}",
                "stdout": "",
                "stderr": "",
                "exit_code": -1
            }
        
        python_cmd = "python" if sys.platform == "win32" else "python3"
        cmd = [python_cmd, str(script_path)]
        if args:
            cmd.extend(args)
        
        try:
            result = subprocess.run(
                cmd,
                capture_output=True,
                text=True,
                timeout=timeout,
                cwd=working_dir
            )
            return {
                "success": result.returncode == 0,
                "stdout": result.stdout,
                "stderr": result.stderr,
                "exit_code": result.returncode
            }
        except subprocess.TimeoutExpired:
            return {
                "success": False,
                "error": f"Timeout after {timeout} seconds",
                "stdout": "",
                "stderr": "",
                "exit_code": -1
            }
        except Exception as e:
            return {
                "success": False,
                "error": str(e),
                "stdout": "",
                "stderr": traceback.format_exc(),
                "exit_code": -1
            }
    
    def get_python_info(self) -> Dict[str, Any]:
        """Get Python environment information."""
        import platform
        return {
            "success": True,
            "version": VERSION,
            "python_version": sys.version,
            "python_executable": sys.executable,
            "platform": platform.platform(),
            "cwd": os.getcwd()
        }
    
    def install_package(self, package: str, timeout: int = 300) -> Dict[str, Any]:
        """
        Install Python package via pip.
        
        Args:
            package: Package name or requirements.txt path
            timeout: Timeout in seconds
            
        Returns:
            Dict with success status and output
        """
        pip_cmd = "pip"
        try:
            result = subprocess.run(
                [pip_cmd, "install", package],
                capture_output=True,
                text=True,
                timeout=timeout
            )
            return {
                "success": result.returncode == 0,
                "stdout": result.stdout,
                "stderr": result.stderr,
                "exit_code": result.returncode
            }
        except subprocess.TimeoutExpired:
            return {
                "success": False,
                "error": f"Timeout after {timeout} seconds",
                "stdout": "",
                "stderr": "",
                "exit_code": -1
            }
        except Exception as e:
            return {
                "success": False,
                "error": str(e),
                "stdout": "",
                "stderr": traceback.format_exc(),
                "exit_code": -1
            }


def main():
    """CLI interface for testing."""
    executor = PythonExecutor()
    
    print("=" * 50)
    print("QuickDesk Python Executor - Test")
    print("=" * 50)
    print()
    
    # Test 1: Get Python Info
    print("1. Testing get_python_info()...")
    result = executor.get_python_info()
    print(json.dumps(result, indent=2))
    print()
    
    # Test 2: Run simple Python code
    print("2. Testing run_python() - simple calculation...")
    result = executor.run_python("print(2 + 2)")
    print(json.dumps(result, indent=2))
    print()
    
    # Test 3: Run more complex code
    print("3. Testing run_python() - list operations...")
    code = """
import json
data = [1, 2, 3, 4, 5]
squared = [x ** 2 for x in data]
print(f'Original: {data}')
print(f'Squared: {squared}')
print(f'Sum: {sum(squared)}')
"""
    result = executor.run_python(code)
    print(json.dumps(result, indent=2))
    print()
    
    # Test 4: Run script (create and run)
    print("4. Testing run_script()...")
    test_script = "/tmp/test_script.py"
    with open(test_script, "w") as f:
        f.write("""
import sys
name = sys.argv[1] if len(sys.argv) > 1 else "World"
print(f"Hello, {name}!")
print(f"Python version: {sys.version}")
""")
    
    result = executor.run_script(test_script, args=["QuickDesk"])
    print(json.dumps(result, indent=2))
    print()
    
    # Test 5: Error handling
    print("5. Testing error handling...")
    result = executor.run_python("raise ValueError('Test error')")
    print(json.dumps(result, indent=2))
    print()
    
    print("=" * 50)
    print("All tests completed!")
    print("=" * 50)


if __name__ == "__main__":
    main()
