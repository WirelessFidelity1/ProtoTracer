"""Exercise the real Win32 message loop with a hidden simulator window.

Run after build.py. The simulator's existing settings file is restored afterward.
"""
import ctypes
from ctypes import wintypes
from pathlib import Path
import subprocess
import time

HERE = Path(__file__).resolve().parent
EXE = HERE / 'build/ProtoTracerSimulator.exe'
SETTINGS = EXE.parent / 'simulator-settings.bin'


def main():
    user32 = ctypes.WinDLL('user32', use_last_error=True)
    callback_type = ctypes.WINFUNCTYPE(wintypes.BOOL, wintypes.HWND, wintypes.LPARAM)
    user32.EnumWindows.argtypes = (callback_type, wintypes.LPARAM)
    user32.GetWindowThreadProcessId.argtypes = (wintypes.HWND, ctypes.POINTER(wintypes.DWORD))
    user32.PostMessageW.argtypes = (wintypes.HWND, wintypes.UINT, wintypes.WPARAM, wintypes.LPARAM)
    original = SETTINGS.read_bytes() if SETTINGS.exists() else None
    # Give this test a known default configuration without changing user settings.
    SETTINGS.write_bytes(bytes([255]) * 512)
    startup = subprocess.STARTUPINFO()
    startup.dwFlags |= subprocess.STARTF_USESHOWWINDOW
    startup.wShowWindow = 0
    process = None
    try:
        process = subprocess.Popen([str(EXE)], cwd=EXE.parent, startupinfo=startup)
        window = None
        deadline = time.monotonic() + 10
        while window is None and time.monotonic() < deadline:
            windows = []

            @callback_type
            def enum(hwnd, _):
                pid = wintypes.DWORD()
                user32.GetWindowThreadProcessId(hwnd, ctypes.byref(pid))
                if pid.value == process.pid:
                    windows.append(hwnd)
                return True

            user32.EnumWindows(enum, 0)
            if windows:
                window = windows[0]
            else:
                assert process.poll() is None, 'Simulator exited before creating a window'
                time.sleep(0.05)
        assert window is not None, 'Simulator did not create a window'

        def post(message, key=0, param=0):
            assert user32.PostMessageW(window, message, key, param), ctypes.get_last_error()

        def press(key):
            post(0x100, key)  # WM_KEYDOWN
            time.sleep(0.08)
            post(0x101, key)  # WM_KEYUP
            time.sleep(0.04)

        press(0x27)  # Right: select face 1
        press(0x0D)  # Enter: save face and advance settings
        press(0x26)  # Up: activities
        press(0x27)  # Right: Pong
        press(0x0D)  # Enter: launch
        post(0x200, 0, (220 << 16) | 150)  # WM_MOUSEMOVE
        post(0x201, 1)  # WM_LBUTTONDOWN
        time.sleep(0.08)
        post(0x202)  # WM_LBUTTONUP
        press(0x26)  # Return to picker
        press(0x26)  # Return to faces
        press(0x71)  # F2: boop
        press(0x72)  # F3: synthetic audio
        post(0x20A, 120 << 16)  # WM_MOUSEWHEEL
        press(ord('B'))
        press(ord('B'))
        assert process.poll() is None, 'Simulator crashed while processing inputs'
        post(0x10)  # WM_CLOSE
        assert process.wait(timeout=10) == 0
        assert SETTINGS.read_bytes()[0] == 1, 'Keyboard selection/save did not reach firmware EEPROM'
        print('PASS: native window, keyboard settings/save, mouse, boop/audio keys, clean close')
    finally:
        if process is not None and process.poll() is None:
            process.terminate()
            process.wait(timeout=5)
        if original is not None:
            SETTINGS.write_bytes(original)
        else:
            SETTINGS.unlink(missing_ok=True)


if __name__ == '__main__':
    main()
