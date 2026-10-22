HUB75 movies
============

## Face shape editor

Double-click **Run Face Editor.cmd** in this folder. Requires Python 3 with
Tcl/Tk (included in the standard Windows Python installer); no extra packages.
The launcher also detects PlatformIO's Python.

The editor opens `NukudeFlat.h`, used by the HUB75 and WS35 projects (its C++
class is named `NukudeFace`). `NukudeFace.h` is a separate model that these
projects do not load. Open header also
supports `GammaFace.h` and other generated headers with the same array format.
Select **Base shape** to reshape the neutral face, or select an expression to
edit its existing morph offsets. Drag cyan vertices; Shift-click selects several
points to move together. Use XY/XZ/YZ views and coordinate fields for depth edits.
The preview slider blends the selected expression; dragging requires 100%.
Mouse wheel zooms, right-drag pans, and Fit view recenters the model.

**Save + backup** writes the header and creates a timestamped `.bak` beside it.
Ctrl+Z undoes edits; closing with unsaved edits asks before discarding them.
Rebuild the simulator (`simulator/Start-Simulator.ps1 -Build`) or firmware to see
the changes there. Restore a backup by copying it over the original header.

This edits existing vertices and expressions, preserving triangle topology,
expression names, and animation code. Base changes carry through to expressions;
expression editing is limited to vertices already affected by that expression.
The view is an orthographic mesh preview, not a simulation of the panel layout,
materials, or combined runtime animations. Adding geometry or new morphs still
requires the full modeling/export workflow.

For a Windows preview with the same faces, menus, movies, and games, see
[`simulator/README.md`](../simulator/README.md). Double-click
`simulator/Run Simulator.cmd` to launch it without a Teensy.

Press Up to open the activity menu. Left/Right cycle Slots, Pong, Movies, Text, Tetris, and Flappy;
Enter or Space opens the selection. In Movies, Left/Right cycle the twelve
available clips, and Enter or Space starts playback. Left/Right also switch
movies during playback. Up returns to the movie picker, then the activity menu,
then faces. Slots spins with Space; Pong retains its USB mouse paddle controls.

`python tools/build_movies.py` regenerates `MoviesData.h` after texture changes.
Install Pillow with `python -m pip install Pillow` for GIF conversion. The generator
also reads `boyksr.gif` from the repository root, fits it within 51x32 without
stretching, preserves its original colors, and samples its timing at the existing 42 ms playback interval.
The generator also includes GIFs in the repository's `gifs` folder, sorted by filename, excluding Jork.
The picker contains DVD, Rick, Gamecube, VaporWave, Boyksr, Boom, Cupholder, Huh,
Jarvis, Rally Saab, Saab Ad, and Vert. Left/Right also cycle values in
the normal face and settings menus, wrapping at either end.

The generator samples every frame to the uncovered 51x32 panel area and losslessly packs
the sampled palette indices with run/literal encoding. Original textures and
palettes are preserved. Playback loops at approximately 24 fps; DVD keeps its
existing 60 fps animation. Jork and Toasters are excluded from Movies. Other source assets remain available in the repository
but are excluded from the picker and packed data. The generator verifies each frame's size,
palette indices, and compression round trip.

Press B on any screen to toggle a full-panel Windows 8-style joke blue screen with a sad face and boop error message.
Press B again to return to the previous screen. Holding B does not retrigger it.

Text accepts up to 128 printable ASCII characters, including spaces and punctuation.
Backspace edits; Enter starts scrolling right to left independently across the full 64-pixel width of each panel
and loops the message. Enter again pauses for editing; typing also resumes editing.
Up or Escape returns to the activity menu. B types a letter while in Text.
The input editor appears only on the OLED; the outer panels are blank during editing.
During playback both panels show the same readable message, and the OLED previews it.
Slots also spins on a new boop when the BOOP setting is enabled. Holding the sensor does not repeat spins, and boops during a spin are ignored.

The activity menu pages automatically after four entries; continue Left/Right
past Text to reach Tetris and Flappy. Up always returns from a game.

Tetris: Left/Right move, Enter rotates with wall kicks, Down soft-drops one row,
and Space hard-drops. Full rows clear and award points; gravity increases with
score. Enter or Space restarts after game over.

Dino: Space, Enter, left mouse click, or a new boop starts/jumps and restarts
after game over. Enable BOOP in settings for sensor control. Release the sensor
between boops; holding it does not repeat jumps.

Flappy: Space, Enter, or a left mouse click starts/flaps. Avoid pipes and the
floor/ceiling; each passed pipe scores a point. The same controls restart after
losing. Both new games use the existing 51x32 visible game area and OLED preview.
