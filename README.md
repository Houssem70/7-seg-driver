# 7-Segment Linux Kernel Platform Driver

This is a Linux kernel *platform driver* for controlling a common-cathode 7-segment display via GPIO on a Raspberry Pi.

✅ **Tested on:**  
- **Raspberry Pi 3 Model B**  
- **Raspbian OS (32-bit, based on Debian)**  
- **Kernel version: 6.8.0-64-generic**

## 🛠️ Device Tree Overlay: Build & Deploy

You should already have `seg7-overlay.dts` in the project root. To compile and install it:

```bash
# Compile the overlay
dtc -@ -I dts -O dtb -o seg7.dtbo seg7-overlay.dts

# Copy into Raspberry Pi overlays directory
sudo cp seg7.dtbo /boot/overlays/

# Enable on next boot
echo "dtoverlay=seg7" | sudo tee -a /boot/config.txt

# Reboot to apply
sudo reboot
```

After reboot, the kernel will automatically bind your driver to the `rpi,seg7` node.

## 🔧 Build the Kernel Module

```bash
# 1. Clone the repository
git clone [https://github.com/Houssem70/7-seg-driver](https://github.com/Houssem70/7-seg-driver).git
cd 7-seg-driver
```

### On the Raspberry Pi

If you compile **on** your Raspberry Pi, simply run:

```bash
make
```

This will use the native GCC located in `/usr/bin`.

### Cross‑compiling from x86 → ARM

If you’re on a desktop and want to build for the Pi’s ARM core, invoke:

```bash
make ARCH=arm CROSS_COMPILE=arm-linux-gnueabihf-
```

- `ARCH=arm` tells the kernel build system you’re targeting ARM.  
- `CROSS_COMPILE=arm-linux-gnueabihf-` prefixes compiler/linker commands 
  (e.g. `$(CROSS_COMPILE)gcc` → `arm-linux-gnueabihf-gcc`).

After compilation, insert the module:

```bash
sudo insmod seg7_platform.ko
```

To remove the module:

```bash
sudo rmmod seg7_platform
```

Check the kernel logs for output:

```bash
dmesg | tail
```

## 🚀 Usage

Once the driver is loaded and bound, you will have:

### Character Device

- **Device node**: `/dev/sevenseg`  
- **Write** a digit (0–9) to update the display:
  ```bash
  echo 5 > /dev/sevenseg
  ```
- **Read** the current digit:
  ```bash
  cat /dev/sevenseg
  ```

### Sysfs Attribute

```
/sys/class/sevenseg/sevenseg/value
```

- **Write** to change the displayed digit:
  ```bash
  echo 3 > /sys/class/sevenseg/sevenseg/value
  ```
- **Read** the current digit:
  ```bash
  cat /sys/class/sevenseg/sevenseg/value
  ```

## 🧪 Test Application

A simple user-space C program `testApp.c` iterates from 0 to 9, writes each digit to the driver, reads it back, and prints the value.

### Compile & Run

```bash
gcc -o testApp testApp.c
sudo ./testApp
```

## 📂 Device Tree Binding

See [docs/seg7-binding.txt](docs/seg7-binding.txt) for the full binding specification.

## 📌 Notes

- The platform driver binds via `compatible = "rpi,seg7"`.
- GPIOs 4–11 control segments A–G and the decimal point.
- Ensure no conflicting overlays (SPI, I2C, etc.) use these pins.

## Enjoy your blinking digits!