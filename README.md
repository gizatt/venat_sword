
## Setup

Follow this, approximately
https://medium.com/@alwint3r/working-with-seeed-xiao-ble-sense-and-platformio-ide-5c4da3ab42a3

But in order to get platformio to not overwrite that package, first build the project with the write board target; allow platformio to download `framework-arduino-mbed` (version ~3.1.1 for me); then paste the specified package over it, overwriting everywhere. That happened to work.

If having flashing problems, be sure to close *everything* that could be occupying serial ports -- including Cura, Arduino agent!