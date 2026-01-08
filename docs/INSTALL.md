# Installation Guide - PowerA Switch Controller Driver

Detailed step-by-step installation instructions for the PowerA Linux driver.

## Table of Contents

1. [Prerequisites](#prerequisites)
2. [Download and Build](#download-and-build)
3. [Temporary Installation](#temporary-installation)
4. [Permanent Installation](#permanent-installation)
5. [Verification](#verification)
6. [Uninstallation](#uninstallation)
7. [Troubleshooting](#troubleshooting)

## Prerequisites

### Check Your Kernel Version

```bash
uname -r
```

Required: Linux 4.19 or newer

### Install Build Tools

#### Debian/Ubuntu

```bash
sudo apt-get update
sudo apt-get install -y \
    build-essential \
    linux-headers-$(uname -r) \
    git \
    dkms
```

#### Fedora/RHEL/CentOS

```bash
sudo dnf install -y \
    kernel-devel \
    kernel-headers \
    gcc \
    make \
    git \
    dkms
```

#### Arch Linux

```bash
sudo pacman -Syu
sudo pacman -S --needed \
    linux-headers \
    base-devel \
    git \
    dkms
```

#### openSUSE

```bash
sudo zypper install -y \
    kernel-devel \
    gcc \
    make \
    git \
    dkms
```

### Verify Kernel Headers

```bash
ls /lib/modules/$(uname -r)/build
```

If this directory doesn't exist, reinstall kernel headers for your distribution.

## Download and Build

### Option 1: Clone from GitHub

```bash
# Clone the repository
git clone https://github.com/raulhn/powera-switch-driver.git
cd powera-switch-driver

# Build the module
make

# Verify the module was built
ls -lh hid-powera.ko
```

### Option 2: Download Release

```bash
# Download latest release
wget https://github.com/raulhn/powera-switch-driver/archive/refs/heads/main.zip
unzip main.zip
cd powera-switch-driver-main

# Build
make
```

### Build Output

Successful build should show:

```
make -C /lib/modules/6.x. x-generic/build M=/path/to/powera-switch-driver modules
make[1]: Entering directory '/usr/src/linux-headers-6.x.x-generic'
  CC [M]  /path/to/powera-switch-driver/hid-powera.o
  MODPOST /path/to/powera-switch-driver/Module.symvers
  CC [M]  /path/to/powera-switch-driver/hid-powera.mod. o
  LD [M]  /path/to/powera-switch-driver/hid-powera.ko
make[1]: Leaving directory '/usr/src/linux-headers-6.x.x-generic'
```

## Temporary Installation

For testing without permanent installation:

### Load the Module

```bash
# Navigate to build directory
cd powera-switch-driver

# Load the module
sudo insmod hid-powera.ko
```

### Check if Loaded

```bash
# Verify module is loaded
lsmod | grep powera

# Check kernel messages
dmesg | tail -20 | grep powera
```

Expected output:

```
hid_powera             16384  0
```

### Test with Your Controller

1. Plug in the PowerA controller
2. Check recognition:

```bash
dmesg | grep -i powera
```

Expected output:

```
[  123.456] hid-powera 0003:20D6:C006.0001: PowerA controller probe
[  123.457] hid-powera 0003:20D6:C006.0001: PowerA controller initialized
```

### Unload When Done

```bash
sudo rmmod hid_powera
```

## Permanent Installation

### Method 1: Using make install

```bash
cd powera-switch-driver

# Install module
sudo make install

# Load the module
sudo modprobe hid-powera

# Verify
lsmod | grep powera
```

### Method 2: Manual Installation

```bash
# Copy module to kernel modules directory
sudo cp hid-powera.ko /lib/modules/$(uname -r)/kernel/drivers/hid/

# Update module dependencies
sudo depmod -a

# Load module
sudo modprobe hid-powera
```

### Method 3: DKMS Installation (Recommended)

DKMS automatically rebuilds the module when you update your kernel.

#### Create DKMS Configuration

```bash
cd powera-switch-driver

# Create dkms.conf
cat > dkms.conf << 'EOF'
PACKAGE_NAME="hid-powera"
PACKAGE_VERSION="0.1.0"
BUILT_MODULE_NAME[0]="hid-powera"
DEST_MODULE_LOCATION[0]="/kernel/drivers/hid"
AUTOINSTALL="yes"
EOF
```

#### Install with DKMS

```bash
# Copy to DKMS source directory
sudo mkdir -p /usr/src/hid-powera-0.1.0
sudo cp -r * /usr/src/hid-powera-0.1.0/

# Add to DKMS
sudo dkms add -m hid-powera -v 0.1.0

# Build with DKMS
sudo dkms build -m hid-powera -v 0.1.0

# Install with DKMS
sudo dkms install -m hid-powera -v 0.1.0

# Load the module
sudo modprobe hid-powera
```

### Load Module at Boot

#### systemd Method

```bash
# Create module load configuration
echo "hid-powera" | sudo tee /etc/modules-load.d/hid-powera.conf

# Verify
cat /etc/modules-load. d/hid-powera.conf
```

#### Traditional Method

```bash
# Add to /etc/modules
echo "hid-powera" | sudo tee -a /etc/modules
```

### Blacklist Conflicting Drivers (if needed)

If another driver is claiming the device:

```bash
# Create blacklist file
sudo tee /etc/modprobe. d/powera-blacklist.conf << EOF
# Blacklist generic HID for PowerA controller
blacklist hid_generic
EOF

# Regenerate initramfs
sudo update-initramfs -u  # Debian/Ubuntu
sudo dracut -f            # Fedora/RHEL
sudo mkinitcpio -P        # Arch Linux
```

## Verification

### Check Module Status

```bash
# Module loaded?
lsmod | grep powera

# Module info
modinfo hid-powera

# Kernel ring buffer
dmesg | grep powera
```

### Check Device

```bash
# USB device present?
lsusb | grep 20d6

# Input devices
ls -l /dev/input/by-id/ | grep -i powera

# HID raw device
ls -l /dev/hidraw*

# Detailed device info
udevadm info --name=/dev/input/eventX --attribute-walk | grep -i powera
```

### Test Input

```bash
# Install test tools
sudo apt-get install evtest joystick  # Debian/Ubuntu

# Test with evtest
sudo evtest

# Or test with jstest
jstest /dev/input/js0
```

## Un
