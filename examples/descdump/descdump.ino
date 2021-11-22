/*
 * MIT License
 *
 * Copyright (c) 2021 touchgadgetdev@gmail.com
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include <usb/usb_host.h>
#include "show_desc.hpp"
#include "usbhhelp.hpp"

void show_config_desc_full(const usb_config_desc_t *config_desc)
{
#if 0
  // Hex dump full config descriptor so it can be fed into
  // http://eleccelerator.com/usbdescreqparser/
  const uint8_t *val = &config_desc->val[0];
  for (int i = 0; i < config_desc->wTotalLength; i+=16) {
    if ((i + 15) < config_desc->wTotalLength) {
      ESP_LOGI("", "%02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X",
          val[i]  , val[i+1], val[i+2], val[i+3],
          val[i+4], val[i+5], val[i+6], val[i+7],
          val[i+8], val[i+9], val[i+10], val[i+11],
          val[i+12], val[i+13], val[i+14], val[i+15]);
    }
    else {
      for (;i < config_desc->wTotalLength; i++) {
        ESP_LOGI("", "%02X", val[i]);
      }
      break;
    }
  }
#endif

  // Full decode of config desc.
  const uint8_t *p = &config_desc->val[0];
  static uint8_t USB_Class = 0;
  uint8_t bLength;
  for (int i = 0; i < config_desc->wTotalLength; i+=bLength, p+=bLength) {
    bLength = *p;
    if ((i + bLength) <= config_desc->wTotalLength) {
      const uint8_t bDescriptorType = *(p + 1);
      switch (bDescriptorType) {
        case USB_B_DESCRIPTOR_TYPE_DEVICE:
          ESP_LOGI("", "USB Device Descriptor should not appear in config");
          break;
        case USB_B_DESCRIPTOR_TYPE_CONFIGURATION:
          show_config_desc(p);
          break;
        case USB_B_DESCRIPTOR_TYPE_STRING:
          ESP_LOGI("", "USB string desc TBD");
          break;
        case USB_B_DESCRIPTOR_TYPE_INTERFACE:
          USB_Class = show_interface_desc(p);
          break;
        case USB_B_DESCRIPTOR_TYPE_ENDPOINT:
          show_endpoint_desc(p);
          break;
        case USB_B_DESCRIPTOR_TYPE_DEVICE_QUALIFIER:
          // Should not be in config?
          ESP_LOGI("", "USB device qual desc TBD");
          break;
        case USB_B_DESCRIPTOR_TYPE_OTHER_SPEED_CONFIGURATION:
          // Should not be in config?
          ESP_LOGI("", "USB Other Speed TBD");
          break;
        case USB_B_DESCRIPTOR_TYPE_INTERFACE_POWER:
          // Should not be in config?
          ESP_LOGI("", "USB Interface Power TBD");
          break;
        case USB_B_DESCRIPTOR_TYPE_OTG:
          // Should not be in config?
          ESP_LOGI("", "USB OTG TBD");
          break;
        case USB_B_DESCRIPTOR_TYPE_DEBUG:
          // Should not be in config?
          ESP_LOGI("", "USB DEBUG TBD");
          break;
        case USB_B_DESCRIPTOR_TYPE_INTERFACE_ASSOCIATION:
          show_interface_assoc(p);
          break;
        // Class specific descriptors have overlapping values.
        case 0x21:
          switch (USB_Class) {
            case USB_CLASS_HID:
              show_hid_desc(p);
              break;
            case USB_CLASS_APP_SPEC:
              ESP_LOGI("", "App Spec Class descriptor TBD");
              break;
            default:
              ESP_LOGI("", "Unknown USB Descriptor Type: 0x%02x Class: 0x%02x", bDescriptorType, USB_Class);
              break;
          }
          break;
        case 0x22:
          switch (USB_Class) {
            default:
              ESP_LOGI("", "Unknown USB Descriptor Type: 0x%02x Class: 0x%02x", bDescriptorType, USB_Class);
              break;
          }
          break;
        case 0x23:
          switch (USB_Class) {
            default:
              ESP_LOGI("", "Unknown USB Descriptor Type: 0x%02x Class: 0x%02x", bDescriptorType, USB_Class);
              break;
          }
          break;
        case 0x24:
          switch (USB_Class) {
            case USB_CLASS_AUDIO:
              ESP_LOGI("", "Audio Class Descriptor 0x24 TBD");
              break;
            case USB_CLASS_COMM:
              ESP_LOGI("", "Comm Class CS_INTERFACE 0x24 TBD");
              break;
            default:
              ESP_LOGI("", "Unknown USB Descriptor Type: 0x%02x Class: 0x%02x", bDescriptorType, USB_Class);
              break;
          }
          break;
        case 0x25:
          switch (USB_Class) {
            case USB_CLASS_AUDIO:
              ESP_LOGI("", "Audio Class Descriptor 0x25 TBD");
              break;
            case USB_CLASS_COMM:
              ESP_LOGI("", "Comm Class CS_ENDPOINT 0x25 TBD");
              break;
            default:
              ESP_LOGI("", "Unknown USB Descriptor Type: 0x%02x Class: 0x%02x", bDescriptorType, USB_Class);
              break;
          }
          break;
        default:
          ESP_LOGI("", "Unknown USB Descriptor Type: 0x%02x Class: 0x%02x", bDescriptorType, USB_Class);
          break;
      }
    }
    else {
      ESP_LOGI("", "USB Descriptor invalid");
      return;
    }
  }
}

void setup()
{
  usbh_setup(show_config_desc_full);
}

void loop()
{
  usbh_task();
}
