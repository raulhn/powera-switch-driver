// SPDX-License-Identifier: GPL-2.0+
/*
 * HID driver for PowerA controllers for Nintendo Switch
 *
 * Copyright (c) 2026 Raul
 */

#include <linux/device.h>
#include <linux/hid.h>
#include <linux/input.h>
#include <linux/module.h>
#include <linux/spinlock.h>

#define POWERA_VENDOR_ID    0x20d6
#define POWERA_PRODUCT_ID   0xc006

#define POWERA_REPORT_SIZE  12

/* Button bits in byte 1 (data[1]) */
#define POWERA_BTN_B        0x01
#define POWERA_BTN_A        0x02
#define POWERA_BTN_Y        0x04
#define POWERA_BTN_X        0x08
#define POWERA_BTN_L        0x10
#define POWERA_BTN_R        0x20
#define POWERA_BTN_ZL       0x40
#define POWERA_BTN_ZR       0x80

/* Button bits in byte 2 (data[2]) */
#define POWERA_BTN_MINUS    0x01
#define POWERA_BTN_PLUS     0x02
#define POWERA_BTN_LSTICK   0x04
#define POWERA_BTN_RSTICK   0x08
#define POWERA_BTN_HOME     0x10
#define POWERA_BTN_CAPTURE  0x20

/* D-pad values in byte 3 (data[3]) */
#define POWERA_DPAD_UP      0x00
#define POWERA_DPAD_UPRIGHT 0x01
#define POWERA_DPAD_RIGHT   0x02
#define POWERA_DPAD_DNRIGHT 0x03
#define POWERA_DPAD_DOWN    0x04
#define POWERA_DPAD_DNLEFT  0x05
#define POWERA_DPAD_LEFT    0x06
#define POWERA_DPAD_UPLEFT  0x07
#define POWERA_DPAD_NEUTRAL 0x08

struct powera_ctlr {
    struct hid_device *hdev;
    struct input_dev *input;
    spinlock_t lock;
    bool initialized;
};

static int powera_hid_event(struct hid_device *hdev,
                            struct hid_report *report, u8 *data, int len)
{
    struct powera_ctlr *ctlr = hid_get_drvdata(hdev);
    struct input_dev *input = ctlr->input;
    unsigned long flags;
    u8 buttons1, buttons2, dpad;
    u16 lstick_x, lstick_y, rstick_x, rstick_y;

    if (!ctlr->initialized)
        return 0;

    /* Check for valid report */
    if (len != POWERA_REPORT_SIZE || data[0] != 0x3f)
        return 0;

    spin_lock_irqsave(&ctlr->lock, flags);

    /* Parse buttons from bytes 1-2 */
    buttons1 = data[1];
    buttons2 = data[2];

    /* Face buttons (byte 1) */
    input_report_key(input, BTN_SOUTH, buttons1 & POWERA_BTN_B);
    input_report_key(input, BTN_EAST, buttons1 & POWERA_BTN_A);
    input_report_key(input, BTN_WEST, buttons1 & POWERA_BTN_Y);
    input_report_key(input, BTN_NORTH, buttons1 & POWERA_BTN_X);
    
    /* Shoulder buttons (byte 1) */
    input_report_key(input, BTN_TL, buttons1 & POWERA_BTN_L);
    input_report_key(input, BTN_TR, buttons1 & POWERA_BTN_R);
    input_report_key(input, BTN_TL2, buttons1 & POWERA_BTN_ZL);
    input_report_key(input, BTN_TR2, buttons1 & POWERA_BTN_ZR);
    
    /* System buttons (byte 2) */
    input_report_key(input, BTN_SELECT, buttons2 & POWERA_BTN_MINUS);
    input_report_key(input, BTN_START, buttons2 & POWERA_BTN_PLUS);
    input_report_key(input, BTN_THUMBL, buttons2 & POWERA_BTN_LSTICK);
    input_report_key(input, BTN_THUMBR, buttons2 & POWERA_BTN_RSTICK);
    input_report_key(input, BTN_MODE, buttons2 & POWERA_BTN_HOME);
    input_report_key(input, BTN_Z, buttons2 & POWERA_BTN_CAPTURE);

    /* Parse D-pad (byte 3) */
    dpad = data[3];
    
    input_report_abs(input, ABS_HAT0X,
                    (dpad == POWERA_DPAD_LEFT || 
                     dpad == POWERA_DPAD_UPLEFT || 
                     dpad == POWERA_DPAD_DNLEFT) ? -1 : 
                    (dpad == POWERA_DPAD_RIGHT || 
                     dpad == POWERA_DPAD_UPRIGHT || 
                     dpad == POWERA_DPAD_DNRIGHT) ? 1 : 0);
    
    input_report_abs(input, ABS_HAT0Y,
                    (dpad == POWERA_DPAD_UP || 
                     dpad == POWERA_DPAD_UPLEFT || 
                     dpad == POWERA_DPAD_UPRIGHT) ? -1 :
                    (dpad == POWERA_DPAD_DOWN || 
                     dpad == POWERA_DPAD_DNLEFT || 
                     dpad == POWERA_DPAD_DNRIGHT) ? 1 : 0);

    /* Parse 16-bit stick values (little-endian) */
    /* Bytes 4-5: Left stick X */
    /* Bytes 6-7: Left stick Y */
    lstick_x = data[4] | (data[5] << 8);
    lstick_y = data[6] | (data[7] << 8);
    
    /* Scale from 16-bit (0-65535) to 8-bit (0-255) */
    u8 lx = lstick_x >> 8;
    u8 ly = lstick_y >> 8;
    
    /* Bytes 8-9: Right stick X */
    /* Bytes 10-11: Right stick Y */
    rstick_x = data[8] | (data[9] << 8);
    rstick_y = data[10] | (data[11] << 8);
    
    u8 rx = rstick_x >> 8;
    u8 ry = rstick_y >> 8;

    /* Report stick positions (Y axis inverted for Linux convention) */
    input_report_abs(input, ABS_X, lx);
    input_report_abs(input, ABS_Y, 255 - ly);
    input_report_abs(input, ABS_RX, rx);
    input_report_abs(input, ABS_RY, 255 - ry);

    input_sync(input);
    spin_unlock_irqrestore(&ctlr->lock, flags);

    return 0;
}

static int powera_input_create(struct powera_ctlr *ctlr)
{
    struct hid_device *hdev = ctlr->hdev;
    struct input_dev *input;
    int ret;

    input = devm_input_allocate_device(&hdev->dev);
    if (!input)
        return -ENOMEM;

    ctlr->input = input;
    input->name = "PowerA Advantage Wireless Controller";
    input->phys = hdev->phys;
    input->dev.parent = &hdev->dev;
    input->id.bustype = hdev->bus;
    input->id.vendor = hdev->vendor;
    input->id. product = hdev->product;
    input->id.version = hdev->version;

    /* Face buttons */
    input_set_capability(input, EV_KEY, BTN_SOUTH);  /* B */
    input_set_capability(input, EV_KEY, BTN_EAST);   /* A */
    input_set_capability(input, EV_KEY, BTN_WEST);   /* Y */
    input_set_capability(input, EV_KEY, BTN_NORTH);  /* X */
    
    /* Shoulder buttons */
    input_set_capability(input, EV_KEY, BTN_TL);     /* L */
    input_set_capability(input, EV_KEY, BTN_TR);     /* R */
    input_set_capability(input, EV_KEY, BTN_TL2);    /* ZL */
    input_set_capability(input, EV_KEY, BTN_TR2);    /* ZR */
    
    /* System buttons */
    input_set_capability(input, EV_KEY, BTN_SELECT); /* - */
    input_set_capability(input, EV_KEY, BTN_START);  /* + */
    input_set_capability(input, EV_KEY, BTN_THUMBL); /* L Stick */
    input_set_capability(input, EV_KEY, BTN_THUMBR); /* R Stick */
    input_set_capability(input, EV_KEY, BTN_MODE);   /* Home */
    input_set_capability(input, EV_KEY, BTN_Z);      /* Capture */

    /* D-pad */
    input_set_abs_params(input, ABS_HAT0X, -1, 1, 0, 0);
    input_set_abs_params(input, ABS_HAT0Y, -1, 1, 0, 0);

    /* Analog sticks */
    input_set_abs_params(input, ABS_X, 0, 255, 2, 4);
    input_set_abs_params(input, ABS_Y, 0, 255, 2, 4);
    input_set_abs_params(input, ABS_RX, 0, 255, 2, 4);
    input_set_abs_params(input, ABS_RY, 0, 255, 2, 4);

    ret = input_register_device(input);
    if (ret) {
        hid_err(hdev, "Failed to register input device: %d\n", ret);
        return ret;
    }

    return 0;
}

static bool is_powera_controller(struct hid_device *hdev)
{
    /* Check by name for Bluetooth devices that report wrong VID/PID */
    if (strstr(hdev->name, "Lic3 Pro Controller") ||
        strstr(hdev->name, "PowerA")) {
        return true;
    }
    
    /* Check by VID/PID for USB devices */
    if (hdev->vendor == POWERA_VENDOR_ID && hdev->product == POWERA_PRODUCT_ID) {
        return true;
    }
    
    return false;
}

static int powera_probe(struct hid_device *hdev,
                       const struct hid_device_id *id)
{
    struct powera_ctlr *ctlr;
    int ret;

    if (!is_powera_controller(hdev)) {
        return -ENODEV;
    }

    hid_info(hdev, "PowerA controller detected:  %s\n", hdev->name);

    ctlr = devm_kzalloc(&hdev->dev, sizeof(*ctlr), GFP_KERNEL);
    if (!ctlr)
        return -ENOMEM;

    ctlr->hdev = hdev;
    hid_set_drvdata(hdev, ctlr);
    spin_lock_init(&ctlr->lock);

    ret = hid_parse(hdev);
    if (ret) {
        hid_err(hdev, "HID parse failed\n");
        return ret;
    }

    ret = hid_hw_start(hdev, HID_CONNECT_HIDRAW);
    if (ret) {
        hid_err(hdev, "HID hw start failed\n");
        return ret;
    }

    ret = hid_hw_open(hdev);
    if (ret) {
        hid_err(hdev, "HID hw open failed\n");
        goto err_stop;
    }

    ret = powera_input_create(ctlr);
    if (ret) {
        hid_err(hdev, "Failed to create input device\n");
        goto err_close;
    }

    ctlr->initialized = true;
    hid_info(hdev, "PowerA controller ready\n");
    return 0;

err_close:
    hid_hw_close(hdev);
err_stop:
    hid_hw_stop(hdev);
    return ret;
}

static void powera_remove(struct hid_device *hdev)
{
    hid_info(hdev, "PowerA controller disconnected\n");
    hid_hw_close(hdev);
    hid_hw_stop(hdev);
}

static const struct hid_device_id powera_devices[] = {
    /* USB connection */
    { HID_USB_DEVICE(POWERA_VENDOR_ID, POWERA_PRODUCT_ID) },
    /* Bluetooth connection */
    { HID_BLUETOOTH_DEVICE(POWERA_VENDOR_ID, POWERA_PRODUCT_ID) },
    /* Catch Bluetooth devices with generic VID/PID - filtered in probe */
    { HID_BLUETOOTH_DEVICE(HID_ANY_ID, HID_ANY_ID) },
    { }
};
MODULE_DEVICE_TABLE(hid, powera_devices);

static struct hid_driver powera_driver = {
    .name           = "hid-powera",
    . id_table       = powera_devices,
    .probe          = powera_probe,
    . remove         = powera_remove,
    .raw_event      = powera_hid_event,
};
module_hid_driver(powera_driver);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Raul");
MODULE_DESCRIPTION("HID driver for PowerA controllers for Nintendo Switch");
MODULE_VERSION("2.0");
