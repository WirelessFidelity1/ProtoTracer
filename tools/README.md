HUB75 movies
============

Press Up to open the activity menu. Left/Right cycle Slots, Pong, and Movies;
Enter or Space opens the selection. In Movies, Left/Right cycle the five
available displays, and Enter or Space starts
playback. Left/Right also switch movies during playback. Up returns to the movie
picker, then the activity menu, then faces.

`python tools/build_movies.py` regenerates `MoviesData.h` after texture changes.
The picker contains DVD, PS2, Rick, Gamecube, and VaporWave. Left/Right also cycle values in
the normal face and settings menus, wrapping at either end.

The generator samples every frame to the uncovered 51x32 panel area and losslessly packs
the sampled palette indices with run/literal encoding. Original textures and
palettes are preserved. Playback loops at approximately 24 fps; DVD keeps its
existing 60 fps animation. Other source assets remain available in the repository
but are excluded from the picker and packed data. The generator verifies each frame's size,
palette indices, and compression round trip.

Press B on any screen to toggle a full-panel Windows 8-style joke blue screen with a sad face and boop error message.
Press B again to return to the previous screen. Holding B does not retrigger it.
