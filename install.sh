#!/bin/bash

set -e

echo "========================================="
echo "PowerA Driver Installation Script"
echo "========================================="
echo

# Check if running as root
if [ "$EUID" -ne 0 ]; then
  echo "Please run with sudo:   sudo ./install.sh"
  exit 1
fi

# Check for kernel headers
echo "[1/6] Checking kernel headers..."
if [ ! -d "/lib/modules/$(uname -r)/build" ]; then
  echo "ERROR: Kernel headers not found!"
  echo "Install them with: sudo pacman -S linux-headers"
  exit 1
fi
echo "    ✓ Kernel headers found"

# Compile driver
echo "[2/6] Compiling driver..."
make clean
make
echo "    ✓ Driver compiled"

# Unbind existing devices from hid-generic BEFORE installing
echo "[3/6] Unbinding existing devices from generic driver..."
DEVICE_PATH="/sys/bus/hid/drivers/hid-generic"
for dev in $DEVICE_PATH/0003:000020D6:0000C006.*; do
  if [ -e "$dev" ]; then
    DEVICE=$(basename $dev)
    echo "    Unbinding $DEVICE..."
    echo "$DEVICE" >"$DEVICE_PATH/unbind" 2>/dev/null || true
  fi
done
echo "    ✓ Unbound from generic driver"

# Install driver
echo "[4/6] Installing driver..."
make install
echo "    ✓ Driver installed"

# Load driver
echo "[5/6] Loading driver..."
modprobe -r hid-powera 2>/dev/null || true
modprobe hid-powera
echo "    ✓ Driver loaded"

# Rebind device to our driver
echo "[6/6] Binding device to hid-powera..."
sleep 1
for dev in /sys/bus/hid/devices/0003:000020D6:0000C006.*; do
  if [ -e "$dev" ]; then
    DEVICE=$(basename $dev)
    echo "    Binding $DEVICE to hid-powera..."
    echo "$DEVICE" >/sys/bus/hid/drivers/hid-powera/bind 2>/dev/null || true
  fi
done

# Verify
echo
echo "[7/7] Verifying installation..."
if lsmod | grep -q hid_powera; then
  echo "    ✓ Driver is active"
else
  echo "    ✗ Driver not loaded!"
  exit 1
fi

echo
echo "========================================="
echo "Installation complete!"
echo "========================================="
echo
echo "Device binding status:"
dmesg | tail -10 | grep -i powera || echo "  (Check 'sudo dmesg | grep powera' for details)"
echo
echo "Check if controller works:"
echo "  1. The controller should now be bound to hid-powera"
echo "  2. Run: sudo dmesg | grep powera"
echo "  3. Run: sudo evtest"
echo "  4. Run: ls -la /dev/input/by-id/"
echo
echo "If the controller still uses hid-generic after reconnecting:"
echo "  Run: sudo ./rebind.sh"
echo
