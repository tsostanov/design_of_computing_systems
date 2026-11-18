"""Compile the actual level firmware against a fake bus, then test the PC client."""
from pathlib import Path
import os
import shutil
import subprocess
import sys

root = Path(__file__).resolve().parents[1]
os.chdir(root)
compiler = os.environ.get("CC") or shutil.which("gcc")
if not compiler:
    fallback = Path("C:/msys64/ucrt64/bin/gcc.exe")
    compiler = str(fallback) if fallback.exists() else None
if not compiler:
    raise SystemExit("Install GCC or set CC to its executable")
# Ensure MinGW runtime DLLs can be found when launching the executable.
os.environ["PATH"] = str(Path(compiler).parent) + os.pathsep + os.environ["PATH"]
output = root / "tmp/tests"
output.mkdir(parents=True, exist_ok=True)
exe = output / ("test_level.exe" if os.name == "nt" else "test_level")
modules = ("level_model", "level_protocol", "level_sensor", "level_display",
           "level_graphics", "level_app", "uart_driver", "ring_buffer")
subprocess.run([compiler, "-std=c11", "-Wall", "-Wextra", "-Werror", "-Itests/stubs", "-Itests", "-ICore/Inc",
                "tests/test_level.c", "tests/fake_hal.c",
                *(f"Core/Src/{name}.c" for name in modules), "-lm", "-o", str(exe)], check=True)
subprocess.run([str(exe)], check=True)
subprocess.run([sys.executable, "-m", "unittest", "discover", "-s", "tests", "-p", "test_level_client.py", "-v"], check=True)
