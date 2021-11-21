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

const TickType_t HOST_EVENT_TIMEOUT = 1;
const TickType_t CLIENT_EVENT_TIMEOUT = 1;

usb_host_client_handle_t Client_Handle;
usb_device_handle_t Device_Handle;

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

void _client_event_callback(const usb_host_client_event_msg_t *event_msg, void *arg)
{
  esp_err_t err;
  switch (event_msg->event)
  {
    /**< A new device has been enumerated and added to the USB Host Library */
    case USB_HOST_CLIENT_EVENT_NEW_DEV:
      ESP_LOGI("", "New device address: %d", event_msg->new_dev.address);
      err = usb_host_device_open(Client_Handle, event_msg->new_dev.address, &Device_Handle);
      if (err != ESP_OK) ESP_LOGI("", "usb_host_device_open: %x", err);

      usb_device_info_t dev_info;
      err = usb_host_device_info(Device_Handle, &dev_info);
      if (err != ESP_OK) ESP_LOGI("", "usb_host_device_info: %x", err);
      ESP_LOGI("", "speed: %d dev_addr %d vMaxPacketSize0 %d bConfigurationValue %d",
          dev_info.speed, dev_info.dev_addr, dev_info.bMaxPacketSize0,
          dev_info.bConfigurationValue);

      const usb_device_desc_t *dev_desc;
      err = usb_host_get_device_descriptor(Device_Handle, &dev_desc);
      if (err != ESP_OK) ESP_LOGI("", "usb_host_get_device_desc: %x", err);
      show_dev_desc(dev_desc);

      const usb_config_desc_t *config_desc;
      err = usb_host_get_active_config_descriptor(Device_Handle, &config_desc);
      if (err != ESP_OK) ESP_LOGI("", "usb_host_get_config_desc: %x", err);
      show_config_desc_full(config_desc);
      break;
    /**< A device opened by the client is now gone */
    case USB_HOST_CLIENT_EVENT_DEV_GONE:
      ESP_LOGI("", "Device Gone handle: %p", event_msg->dev_gone.dev_hdl);
      break;
    default:
      ESP_LOGI("", "Unknown value %d", event_msg->event);
      break;
  }
}

void setup()
{
  const usb_host_config_t config = {
    .intr_flags = ESP_INTR_FLAG_LEVEL1,
  };
  esp_err_t err = usb_host_install(&config);
  ESP_LOGI("", "usb_host_install: %x", err);

  usb_host_install(&config);
  const usb_host_client_config_t client_config = {
    .client_event_callback = _client_event_callback,
    .callback_arg = Client_Handle,
    .max_num_event_msg = 5
  };
  err = usb_host_client_register(&client_config, &Client_Handle);
  ESP_LOGI("", "usb_host_client_register: %x", err);
}

void loop()
{
  uint32_t event_flags;

  esp_err_t err = usb_host_lib_handle_events(HOST_EVENT_TIMEOUT, &event_flags);
  if (ESP_ERR_TIMEOUT != err) {
    ESP_LOGI("", "usb_host_list_handle_events: %x flags: %x", err, event_flags);
  }

  err = usb_host_client_handle_events(Client_Handle, CLIENT_EVENT_TIMEOUT);
  if (ESP_ERR_TIMEOUT != err) {
    ESP_LOGI("", "usb_host_client_handle_events: %x", err);
  }
}
