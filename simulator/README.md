# Windows simulator

Double-click **Run Simulator.cmd**, or run:

```powershell
.\simulator\Start-Simulator.ps1
```

The launcher builds the app if needed, then opens `simulator/build/ProtoTracerSimulator.exe`.
It also rebuilds automatically when model headers or simulator sources change.
Launch through **Run Simulator.cmd** to pick up saved face edits; opening the
executable directly uses the last build.
The first build needs Python 3 and internet access to install the pinned Zig compiler
(`ziglang==0.16.0`). PlatformIO's Python is detected automatically. Subsequent launches
of the executable require neither Python nor a connected Teensy.

To rebuild after firmware changes:

```powershell
.\simulator\Start-Simulator.ps1 -Build
```

The app compiles the existing HUB75 project, face meshes and materials, rendering
engine, menus, games, text scroller, and packed movies. Hardware adapters capture
the same panel output and OLED drawing commands in a native Windows window.
It displays both 64×32 panels, the 128×64 OLED, 176 accent LEDs, and five antenna LEDs.
Panel orientation, covered-edge margins, menu transitions, and movie timing use
the firmware code. Settings are saved beside the executable in
`simulator-settings.bin`; delete that file to restore defaults.

| Control | Action |
| --- | --- |
| Up | Open activities / return to the previous menu |
| Left / Right | Change settings, activity, or movie selection |
| Enter | Advance settings / launch / game action |
| Space | Game action / restart / start movie |
| Mouse movement | Pong paddle |
| Left click | Game action |
| B | Toggle the blue screen (types B in Text) |
| Typing / Backspace / Enter | Edit and start the text scroller |
| F2 or right mouse button | Hold the simulated boop sensor |
| F3 | Toggle synthetic microphone input |
| Mouse wheel | Adjust synthetic microphone level |

Enable BOOP in the firmware settings for sensor actions. Release between Dino
jumps. F2 remains available during text entry. Mouse inputs are confined to the
simulator window; it does not capture system-wide keyboard or mouse events.

The microphone input is a synthetic signal, not a recording of the Windows
microphone. Fan outputs and I²C/USB electronics are emulated. LED hardware
brightness, refresh artifacts, and physical sensors are not reproduced.
This target simulates the HUB75 configuration; it does not emulate a Teensy CPU
or the WS35 wiring.

Run the deterministic smoke checks with:

```powershell
.\simulator\Start-Simulator.ps1 -Build -SelfTest
```

This exercises the shared face/settings navigation, activities, movies, text,
mouse input, Dino boop controls, and blue-screen toggle, and writes screenshots
and `self-test.txt` under `simulator/build`. Generated build files are ignored
by Git. The compatibility headers are only used by this build, so PlatformIO
continues using the actual Arduino and peripheral libraries.

`python simulator/test_window.py` additionally checks the real Windows message
loop, keyboard settings/save, mouse input, sensor keys, and clean shutdown using
a hidden window. Close any running simulator before testing. The test restores
the simulator's saved settings afterward.

The bundled Picopixel and classic bitmap font data and font structures come
from Adafruit GFX; see `compat/Fonts/ADAFRUIT_LICENSE.txt`.
