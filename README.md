# 7-Segment Linux Kernel Platform Driver

This is a Linux kernel *platform driver* for controlling a common-cathode 7-segment display via GPIO on a Raspberry Pi.

✅ **Tested on:**  
- **Raspberry Pi 3 Model B**  
- **Raspbian OS (32-bit, based on Debian)**  
- **Kernel version:** 6.8.0-64-generic

## 🛠️ Device Tree Overlay: Build & Deploy

Create the overlay source at `seg7-overlay.dts`:

```dts
/dts-v1/;
/plugin/;

&gpio {
    seg7: seg7 {
        compatible = "rpi,seg7";
        label      = "sevenseg";
        gpios = <&gpio 4 0   /* A */,
                 &gpio 5 0   /* B */,
                 &gpio 6 0   /* C */,
                 &gpio 7 0   /* D */,
                 &gpio 8 0   /* E */,
                 &gpio 9 0   /* F */,
                 &gpio 10 0  /* G */,
                 &gpio 11 0>;/* DP */;
        status = "okay";
    };
};
```

Compile & install the overlay:

```bash
cd overlays
dtc -@ -I dts -O dtb -o seg7.dtbo seg7-overlay.dts
sudo cp seg7.dtbo /boot/overlays/
echo "dtoverlay=seg7" | sudo tee -a /boot/config.txt
sudo reboot
```

After reboot, the kernel will automatically bind your driver to the `rpi,seg7` node.

## 🔧 Build the Kernel Module

```bash
make
```

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