"""Build and execute the real firmware modules on the host, then test Python."""
from pathlib import Path
import os
import shutil
import subprocess
import sys

root = Path(__file__).resolve().parents[1]
os.chdir(root)
compiler = os.environ.get("CC") or shutil.which("gcc")
if not compiler:
    raise SystemExit("GCC not found. Install MinGW GCC or set CC to its executable.")
output = root / "tmp/tests"
output.mkdir(parents=True, exist_ok=True)
exe = output / ("test_firmware.exe" if os.name == "nt" else "test_firmware")
sources = ["tests/test_firmware.c", "tests/fake_hal.c"] + [
    f"Core/Src/{name}.c" for name in
    ("ring_buffer", "morse_codec", "morse", "button", "uart_driver", "app")]
subprocess.run([compiler, "-std=c11", "-Wall", "-Wextra", "-Werror",
                "-Itests/stubs", "-Itests", "-ICore/Inc", *sources, "-o", str(exe)], check=True)
subprocess.run([str(exe)], check=True)
os.environ["FIRMWARE_TEST_EXE"] = str(exe)
subprocess.run([sys.executable, "-m", "unittest", "discover", "-s", "tests", "-p", "test_*.py", "-v"], check=True)
