VR Video Player
===============

Android app for VR Video Playback with support for Google Cardboard.

Development state: Basic functionality is working, but it is basically alpha-quality with some features not yet implemented.

Basic usage
-----------

Open a video using the app, or start the app and choose the video from the gallery chooser.

By clicking on the cog wheel icon in the corner, you can switch between supported input & output modes depending on the video format and your hardware:

Output mode
- _Monoscopic (L)_ – display only the left-eye view normally on the phone screen
- _Monoscopic (R)_ – display only the _right_-eye view normally on the phone screen
- _Cardboard_ – if your are using a Google Cardboard device to view the VR video (you can click on _Switch Cardboard viewer_ to configure the used device)

3D video format
- _Monoscopic_ – for 2D video
- _Side-by-side stereo_ – 3D video with views stored side by side
- _Vertical stereo_ – 3D video with views stored above each other
- _Anaglyph, red–cyan_ – 3D video encoded as a color anaglyph using the red–cyan encoding

VR video geometry
- Flat FOV – for normal (non-VR) video
- 180° equirectangular – equirectangular VR video with 180° field of view
- 360° equirectangular – equirectangular VR video with full 360° field of view
- 180° panorama – Flat panoramatic video with 180° field of view
- 360° panorama – Flat panoramatic video with full 360° field of view

By clicking the cross icon in the opposite corner, you can close the application.

By tapping the display, you can seek forward (on the right-hand side of the screen) or backward (on the left-hand side of the screen).

In the VR view, you you can activate (or deactivate) VR controls by looking up; these allow you to control the playback, raise or lower the volume, re-center the VR view, etc.

Attribution
-----------

Includes the following libraries:
- Cardboard SDK library from Google, LLC, licensed under the Apache License, Version 2.0 (see `app/libs/cardboard-sdk/`). See https://developers.google.com/cardboard
- OpenGL Mathematics (GLM) from G-Truc Creation, licensed under The Happy Bunny License or MIT License (see `app/srv/main/cpp/glm/`). See https://glm.g-truc.net/
