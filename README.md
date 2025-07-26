# 7-Segment Linux Kernel Module

This is a simple Linux kernel module for controlling a 7-segment display via GPIO.

## Build & Deploy Device tree
```bash
dtc -@ -I dts -O dtb -o seg7.dtbo seg7-overlay.dts
sudo cp seg7.dtbo /boot/overlays/
echo "dtoverlay=seg7" | sudo tee -a /boot/config.txt
sudo reboot
```

## Build driver
```bash
make
```

