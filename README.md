# 7-Segment Linux Kernel Module

This is a simple Linux kernel module for controlling a 7-segment display using GPIO pins on a Raspberry Pi.

✅ **Tested on:**  
- **Raspberry Pi 3 Model B**  
- **Raspbian OS (32-bit, based on Debian)**

## 🛠️ Device Tree Overlay: Build & Deploy

First, compile the Device Tree overlay and enable it on boot:

```bash
dtc -@ -I dts -O dtb -o seg7.dtbo seg7-overlay.dts
sudo cp seg7.dtbo /boot/overlays/
echo "dtoverlay=seg7" | sudo tee -a /boot/config.txt
sudo reboot
```

## 🔧 Build the Kernel Module

```bash
make
```

After compilation, you can insert the module using:

```bash
sudo insmod seg7.ko
```

And remove it using:

```bash
sudo rmmod seg7
```

Check the kernel logs for output:

```bash
dmesg | tail
```

## 📌 Notes

- The `seg7` overlay configures GPIOs 4 to 11 to control segments A–G and the decimal point.
- This module uses the character device interface (`cdev`) for interaction from user space.
- Make sure your Raspberry Pi GPIO pins are not used by other overlays or services (like SPI, I2C) that could conflict.