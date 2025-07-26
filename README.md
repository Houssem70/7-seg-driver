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

After compilation, insert the module:

```bash
sudo insmod seg7.ko
```

To remove the module:

```bash
sudo rmmod seg7
```

Check the kernel logs for output:

```bash
dmesg | tail
```

## 🚀 Usage

### Character Device Interface

- **Device node**: `/dev/sevenseg`  
- **Write** a digit (0–9) to update the display:
  ```bash
  echo 5 > /dev/sevenseg
  ```
- **Read** the current digit:
  ```bash
  cat /dev/sevenseg
  ```
- Ensure permissions allow access or use `sudo`.

### Sysfs Interface

A sysfs attribute is created under the device's sysfs directory:

```
/sys/class/sevenseg/sevenseg/value
```

- **Write** to sysfs to change the displayed digit:
  ```bash
  echo 3 > /sys/class/sevenseg/sevenseg/value
  ```
- **Read** the current digit from sysfs:
  ```bash
  cat /sys/class/sevenseg/sevenseg/value
  ```

## 📌 Notes

- The `seg7` overlay configures GPIOs 4 to 11 to control segments A–G and the decimal point.
- This module uses:
  - A character device (`/dev/sevenseg`) for simple read/write operations.
  - A sysfs attribute (`value`) under `/sys/class/sevenseg/sevenseg/`.
- Make sure GPIO pins 4–11 are not used by other overlays or services (like SPI, I2C) that could conflict.
- The driver automatically discovers GPIOs via the Device Tree node `compatible = "rpi,seg7"`.