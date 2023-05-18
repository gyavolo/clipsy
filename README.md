# Clipsy
Or how to use CLIPboard for (somewhat) Secure text entrY.

Clipsy is a tool that takes X11 window id as an argument, then finds the process that created that window and reads a line of text from stdin. It "stores" that line to clipboard (actually takes ownership of CLIPBOARD and PRIMARY selections), but restricts it only to the windows of the selected process (or optionally the process and its ancestors). All this allows somewhat secure way to insert text data (like passwords) to a GUI application, without using the potentially vulnerable to keyloggers and sniffers X11 keyboard (including on-screen) and clipboard systems.

Build with `make` or `make PARENTS=1` to allow pasting to windows of the parent process and all of its ancestors. This option requires libprocps library.

Install with `sudo make install`. This will set up the `clipsy` command, as well as a simple virtual keyboard and the `clipsy-vk` script that uses xdotool to select a window with mouse click, then opens the keyboard and pipes its output to `clipsy`. When the user clicks on "Enter", the keyboard is closed and CLIPBOARD or PRIMARY selection can be pasted on the designated window.

Alternatively, `clypsy` can be used standalone by piping-in data from whatever desired, like password managers, etc. For secure direct input, it can be used from a non-X virtual terminal like:
```
export DISPLAY=":0" && clipsy `xdotool selectwindow`
```
Furthermore, executing it on a terminal gives plenty of information about any suspicious clipboard selection requests.

Note: In order to annoy some screenshotters and mouse click loggers, the supplied virtual keyboard blinks briefly (by destroyng the window) after every keypress. This can be turned off by adding -b option to `vkbd-clipsy`. An image is used instead of button widgets and `clipsy-vk` also takes some measures in order to conceal what it really is. For the even more paranoid ones, pressing the "move" button or adding -R option to `vkbd-clipsy` causes it to randomly move around as well. Furthermore, `make install` by default adds a randomized copy of `vkbd-clipsy` with random name, size and file times, readable and executable only by sudo. This can be used instead of the plain `vkbd-vlipsy` to make it harder for any malware to recognize and potentially monitor its process more closely than usual. Regardless of everything, using any kind of X GUI to enter sensitive data is relatively insecure, but the on-screen keyboard is far more convenient than switching back and forth to a virtual terminal.
