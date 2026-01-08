# PowerA Nintendo Switch Controller Driver for Linux

Linux kernel driver for PowerA Advantage Wireless Controller for Nintendo Switch.

## Features

✅ Full button mapping (A, B, X, Y, L, R, ZL, ZR, +, -, Home, Capture)  
✅ Analog stick support (left and right)  
✅ D-pad support  
✅ USB and Bluetooth support  
✅ Automatic controller initialization

## Hardware Support

- **Vendor ID**: 0x20d6
- **Product ID**: 0xc006
- **Name**: PowerA Advantage Wireless Controller for Nintendo Switch 2

## Requirements

- Linux kernel 5.10 or newer
- Kernel headers installed
- GCC compiler
- Make

### Install requirements on Arch Linux:

```bash
sudo pacman -S base-devel linux-headers
```

## Installation

### Quick install:

```bash
chmod +x install.sh
sudo ./install.sh
```

### Manual install:

```bash
# Compile
make

# Install
sudo make install

# Load driver
sudo modprobe hid-powera
```

## Usage

1. **Connect your controller** (USB or Bluetooth)

2. **Verify driver is loaded:**

   ```bash
   lsmod | grep hid_powera
   dmesg | tail -20 | grep -i powera
   ```

3. **Test with evtest:**

   ```bash
   sudo evtest
   # Select the PowerA device and press buttons
   ```

4. **Test with jstest** (if available):
   ```bash
   sudo pacman -S joyutils  # Install if needed
   jstest /dev/input/js0
   ```

## Button Mapping

| Physical Button | Linux Input | SDL GameController |
| --------------- | ----------- | ------------------ |
| A               | BTN_EAST    | A                  |
| B               | BTN_SOUTH   | B                  |
| X               | BTN_NORTH   | X                  |
| Y               | BTN_WEST    | Y                  |
| L               | BTN_TL      | Left Shoulder      |
| R               | BTN_TR      | Right Shoulder     |
| ZL              | BTN_TL2     | Left Trigger       |
| ZR              | BTN_TR2     | Right Trigger      |
| -               | BTN_SELECT  | Back               |
| +               | BTN_START   | Start              |
| Home            | BTN_MODE    | Guide              |
| Capture         | BTN_Z       | Misc               |
| Left Stick      | BTN_THUMBL  | Left Stick Button  |
| Right Stick     | BTN_THUMBR  | Right Stick Button |
| D-pad           | ABS_HAT0X/Y | D-pad              |
| Left Analog     | ABS_X/Y     | Left Stick         |
| Right Analog    | ABS_RX/RY   | Right Stick        |

## Troubleshooting

### Controller not detected

```bash
# Check if controller is connected
lsusb | grep 20d6

# Check hidraw devices
ls -la /dev/hidraw*

# Check which driver is handling it
for dev in /dev/hidraw*; do
    echo "=== $dev ==="
    sudo cat /sys/class/hidraw/$(basename $dev)/device/uevent | grep HID_NAME
done
```

### Driver not loading

```bash
# Check kernel logs
dmesg | grep -i powera

# Try manual load
sudo modprobe hid-powera

# Check for errors
sudo journalctl -xe | grep powera
```

### Bluetooth not working

1.  Disconnect USB cable
2.  Put controller in pairing mode (hold SYNC button)
3.  Pair via bluetoothctl:
    ```bash
    bluetoothctl
    scan on
    pair XX:XX:XX:XX:XX:XX
    connect XX:XX:XX:XX: XX:XX
    ```

### Controller sends no data

The controller may need activation:

```bash
# Unplug and replug USB while pressing HOME button
# OR
# Reconnect via Bluetooth and press HOME button
```

## Uninstall

```bash
sudo make uninstall
sudo modprobe -r hid-powera
```

## Development

### Enable debug messages:

```bash
# Load with debugging
sudo modprobe hid-powera
echo 8 | sudo tee /proc/sys/kernel/printk

# Watch kernel logs
sudo dmesg -w | grep powera
```

### Capture raw HID data:

```bash
# Find your hidraw device
ls /dev/hidraw*

# Capture data (press buttons!)
sudo cat /dev/hidraw1 | hexdump -C
```

## Known Issues

- [ ] Controller may not send data immediately after connection
- [ ] Bluetooth pairing requires manual steps
- [ ] Vibration/rumble not yet implemented
- [ ] Battery status not yet reported

## Contributing

Contributions welcome! If you improve the button mappings or fix issues, please share your changes.

## License

GPL-2.0+ (same as Linux kernel)

## Credits

- Based on `hid-nintendo` driver by Daniel J. Ogorchock
- Developed for PowerA Advantage Wireless Controller

## References

- [Linux HID subsystem documentation](https://www.kernel.org/doc/html/latest/hid/index.html)
- [Nintendo Switch reverse engineering](https://github.com/dekuNukem/Nintendo_Switch_Reverse_Engineering)
