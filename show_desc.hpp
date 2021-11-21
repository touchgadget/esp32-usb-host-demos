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

const size_t USB_HID_DESC_SIZE = 9;

typedef union {
    struct {
        uint8_t bLength;                    /**< Size of the descriptor in bytes */
        uint8_t bDescriptorType;            /**< Constant name specifying type of HID descriptor. */
        uint16_t bcdHID;                    /**< USB HID Specification Release Number in Binary-Coded Decimal (i.e., 2.10 is 210H) */
        uint8_t bCountryCode;               /**< Numeric expression identifying country code of the localized hardware. */
        uint8_t bNumDescriptor;             /**< Numeric expression specifying the number of class descriptors. */
        uint8_t bHIDDescriptorType;         /**< Constant name identifying type of class descriptor. See Section 7.1.2: Set_Descriptor Request for a table of class descriptor constants. */
        uint16_t wHIDDescriptorLength;      /**< Numeric expression that is the total size of the Report descriptor. */
        uint8_t bHIDDescriptorTypeOpt;      /**< Optional constant name identifying type of class descriptor. See Section 7.1.2: Set_Descriptor Request for a table of class descriptor constants. */
        uint16_t wHIDDescriptorLengthOpt;   /**< Optional numeric expression that is the total size of the Report descriptor. */
    } USB_DESC_ATTR;
    uint8_t val[USB_HID_DESC_SIZE];
} usb_hid_desc_t;

void show_dev_desc(const usb_device_desc_t *dev_desc)
{
  ESP_LOGI("", "bLength: %d", dev_desc->bLength);
  ESP_LOGI("", "bDescriptorType(device): %d", dev_desc->bDescriptorType);
  ESP_LOGI("", "bcdUSB: 0x%x", dev_desc->bcdUSB);
  ESP_LOGI("", "bDeviceClass: 0x%02x", dev_desc->bDeviceClass);
  ESP_LOGI("", "bDeviceSubClass: 0x%02x", dev_desc->bDeviceSubClass);
  ESP_LOGI("", "bDeviceProtocol: 0x%02x", dev_desc->bDeviceProtocol);
  ESP_LOGI("", "bMaxPacketSize0: %d", dev_desc->bMaxPacketSize0);
  ESP_LOGI("", "idVendor: 0x%x", dev_desc->idVendor);
  ESP_LOGI("", "idProduct: 0x%x", dev_desc->idProduct);
  ESP_LOGI("", "bcdDevice: 0x%x", dev_desc->bcdDevice);
  ESP_LOGI("", "iManufacturer: %d", dev_desc->iManufacturer);
  ESP_LOGI("", "iProduct: %d", dev_desc->iProduct);
  ESP_LOGI("", "iSerialNumber: %d", dev_desc->iSerialNumber);
  ESP_LOGI("", "bNumConfigurations: %d", dev_desc->bNumConfigurations);
}

void show_config_desc(const void *p)
{
  const usb_config_desc_t *config_desc = (const usb_config_desc_t *)p;

  ESP_LOGI("", "bLength: %d", config_desc->bLength);
  ESP_LOGI("", "bDescriptorType(config): %d", config_desc->bDescriptorType);
  ESP_LOGI("", "wTotalLength: %d", config_desc->wTotalLength);
  ESP_LOGI("", "bNumInterfaces: %d", config_desc->bNumInterfaces);
  ESP_LOGI("", "bConfigurationValue: %d", config_desc->bConfigurationValue);
  ESP_LOGI("", "iConfiguration: %d", config_desc->iConfiguration);
  ESP_LOGI("", "bmAttributes(%s%s%s): 0x%02x",
      (config_desc->bmAttributes & USB_BM_ATTRIBUTES_SELFPOWER)?"Self Powered":"",
      (config_desc->bmAttributes & USB_BM_ATTRIBUTES_WAKEUP)?", Remote Wakeup":"",
      (config_desc->bmAttributes & USB_BM_ATTRIBUTES_BATTERY)?", Battery Powered":"",
      config_desc->bmAttributes);
  ESP_LOGI("", "bMaxPower: %d = %d mA", config_desc->bMaxPower, config_desc->bMaxPower*2);
}

uint8_t show_interface_desc(const void *p)
{
  const usb_intf_desc_t *intf = (const usb_intf_desc_t *)p;

  ESP_LOGI("", "bLength: %d", intf->bLength);
  ESP_LOGI("", "bDescriptorType (interface): %d", intf->bDescriptorType);
  ESP_LOGI("", "bInterfaceNumber: %d", intf->bInterfaceNumber);
  ESP_LOGI("", "bAlternateSetting: %d", intf->bAlternateSetting);
  ESP_LOGI("", "bNumEndpoints: %d", intf->bNumEndpoints);
  ESP_LOGI("", "bInterfaceClass: 0x%02x", intf->bInterfaceClass);
  ESP_LOGI("", "bInterfaceSubClass: 0x%02x", intf->bInterfaceSubClass);
  ESP_LOGI("", "bInterfaceProtocol: 0x%02x", intf->bInterfaceProtocol);
  ESP_LOGI("", "iInterface: %d", intf->iInterface);
  return intf->bInterfaceClass;
}

void show_endpoint_desc(const void *p)
{
  const usb_ep_desc_t *endpoint = (const usb_ep_desc_t *)p;
  const char *XFER_TYPE_NAMES[] = {
    "Control", "Isochronous", "Bulk", "Interrupt"
  };
  ESP_LOGI("", "bLength: %d", endpoint->bLength);
  ESP_LOGI("", "bDescriptorType (endpoint): %d", endpoint->bDescriptorType);
  ESP_LOGI("", "bEndpointAddress(%s): 0x%02x",
    (endpoint->bEndpointAddress & USB_B_ENDPOINT_ADDRESS_EP_DIR_MASK)?"In":"Out",
    endpoint->bEndpointAddress);
  ESP_LOGI("", "bmAttributes(%s): 0x%02x",
      XFER_TYPE_NAMES[endpoint->bmAttributes & USB_BM_ATTRIBUTES_XFERTYPE_MASK],
      endpoint->bmAttributes);
  ESP_LOGI("", "wMaxPacketSize: %d", endpoint->wMaxPacketSize);
  ESP_LOGI("", "bInterval: %d", endpoint->bInterval);
}

void show_hid_desc(const void *p)
{
  usb_hid_desc_t *hid = (usb_hid_desc_t *)p;
  ESP_LOGI("", "bLength: %d", hid->bLength);
  ESP_LOGI("", "bDescriptorType (HID): %d", hid->bDescriptorType);
  ESP_LOGI("", "bcdHID: 0x%04x", hid->bcdHID);
  ESP_LOGI("", "bCountryCode: %d", hid->bCountryCode);
  ESP_LOGI("", "bNumDescriptor: %d", hid->bNumDescriptor);
  ESP_LOGI("", "bDescriptorType: %d", hid->bHIDDescriptorType);
  ESP_LOGI("", "wDescriptorLength: %d", hid->wHIDDescriptorLength);
  if (hid->bNumDescriptor > 1) {
    ESP_LOGI("", "bDescriptorTypeOpt: %d", hid->bHIDDescriptorTypeOpt);
    ESP_LOGI("", "wDescriptorLengthOpt: %d", hid->wHIDDescriptorLengthOpt);
  }
}

void show_interface_assoc(const void *p)
{
  usb_iad_desc_t *iad = (usb_iad_desc_t *)p;
  ESP_LOGI("", "bLength: %d", iad->bLength);
  ESP_LOGI("", "bDescriptorType: %d", iad->bDescriptorType);
  ESP_LOGI("", "bFirstInterface: %d", iad->bFirstInterface);
  ESP_LOGI("", "bInterfaceCount: %d", iad->bInterfaceCount);
  ESP_LOGI("", "bFunctionClass: 0x%02x", iad->bFunctionClass);
  ESP_LOGI("", "bFunctionSubClass: 0x%02x", iad->bFunctionSubClass);
  ESP_LOGI("", "bFunctionProtocol: 0x%02x", iad->bFunctionProtocol);
  ESP_LOGI("", "iFunction: %d", iad->iFunction);
}
