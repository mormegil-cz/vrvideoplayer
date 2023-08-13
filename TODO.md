- perf/power optimization: refactor to VBO

- VR mode controls:
  - Probably shown by head up motion to show/hide controls (and pointer dot)
  - controls:
    - yaw recenter
    - (2D recenter)
    - volume up + down
    - close file and open another (⇒ VR file chooser)
    - jump to start
    - rewind back
    - fast forward
    - pause/resume

- VR file chooser
  - probably a panel of file names/icons(/thumbnails??), plus a panel of options and action buttons
    - Next/Prev paging, OK button, Exit button, input file mode chooser, input layout chooser

- GUI controls in 2D mode??

- video aspect ratio in flat & panorama modes

- Move input mode/layout settings to the file open dialog. (What with the initial app open? Probably
  depends on the default output mode? If VR, skip the current system gallery and just go to the VR
  file chooser. In 2D mode… custom gallery?? Bah… Just allow to switch the settings afterwards in
  this popup menu anyway?)

- maybe VR view locking in some input modes? (e.g. for panoramas, limit to horizontal only; for 180°
  modes, do not (optionally?) allow to move beyond the edge?)
