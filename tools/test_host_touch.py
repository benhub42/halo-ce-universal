"""Runs the C tests of the cheats menu's way through the Android host
(port/android/host/host_touch.c) with the build machine's C compiler and a
stand-in JNI header (port/android/tests/jni_stub)."""
import shutil
import subprocess
from pathlib import Path

import pytest

ROOT = Path(__file__).resolve().parent.parent


def compiler():
    for name in ("clang", "cc", "gcc"):
        path = shutil.which(name)
        if path:
            return path
    return None


def test_host_touch(tmp_path):
    cc = compiler()
    if not cc:
        pytest.skip("no C compiler")
    tests = ROOT / "port" / "android" / "tests"
    binary = tmp_path / "host_touch_test"
    subprocess.run(
        [cc, "-std=c11", "-Wall", "-Wextra", "-Werror", "-O1", "-pthread",
         "-I", str(tests / "jni_stub"),
         str(tests / "host_touch_test.c"),
         str(ROOT / "port" / "android" / "host" / "host_touch.c"),
         "-o", str(binary)],
        check=True,
    )
    result = subprocess.run([str(binary)], capture_output=True, text=True)
    assert result.returncode == 0, result.stdout + result.stderr
