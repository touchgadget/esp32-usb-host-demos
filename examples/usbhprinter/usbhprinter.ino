#include <usb/usb_host.h>
#include "show_desc.hpp"
#include "usbhhelp.hpp"

bool isPrinter = false;
bool isBiDirectional = false;

const size_t PRINTER_OUT_BUFFERS = 8;
usb_transfer_t *PrinterOut = NULL;
usb_transfer_t *PrinterIn = NULL;

static void printer_transfer_cb(usb_transfer_t *transfer)
{
  ESP_LOGI("", "printer_transfer_cb context: %d", transfer->context);
  if (Device_Handle == transfer->device_handle) {
    int in_xfer = transfer->bEndpointAddress & USB_B_ENDPOINT_ADDRESS_EP_DIR_MASK;
    if (transfer->status == 0) {
      if (in_xfer) {
        uint8_t *const p = transfer->data_buffer;
        for (int i = 0; i < transfer->actual_num_bytes; i++) {
          ESP_LOGI("", "printer in: %02x", p[i]);
        }
        esp_err_t err = usb_host_transfer_submit(transfer);
        if (err != ESP_OK) {
          ESP_LOGI("", "usb_host_transfer_submit In fail: %x", err);
        }
      }
    }
    else {
      ESP_LOGI("", "transfer->status %d", transfer->status);
    }
  }
}

void check_interface_desc_printer(const void *p)
{
  const usb_intf_desc_t *intf = (const usb_intf_desc_t *)p;

  // USB Printer Class Specification 1.1
  if ((intf->bInterfaceClass == USB_CLASS_PRINTER) &&
      (intf->bInterfaceSubClass == 1))
  {
    /* Protocol
     * 00 Reserved, undefined.
     * 01 Unidirectional interface.
     * 02 Bi-directional interface.
     * 03 1284.4 compatible bi-directional interface.
     * 04-FEh Reserved for future use.
     * FFh Vendor-specific printers do not use class-specific protocols.
     */
    // No idea how to support 1284.4 so ignore it.
    if ((intf->bInterfaceProtocol == 1) || (intf->bInterfaceProtocol == 2)) {
      isBiDirectional = (intf->bInterfaceProtocol == 2);
      if (isBiDirectional) {
        if (intf->bNumEndpoints < 2) {
          isPrinter = false;
          return;
        }
      }
      else {
        if (intf->bNumEndpoints < 1) {
          isPrinter = false;
          return;
        }
      }
      isPrinter = true;
      ESP_LOGI("", "Claiming a %s-directional printer!", (isBiDirectional)?"bi":"uni");
      esp_err_t err = usb_host_interface_claim(Client_Handle, Device_Handle,
          intf->bInterfaceNumber, intf->bAlternateSetting);
      if (err != ESP_OK) ESP_LOGI("", "usb_host_interface_claim failed: %x", err);
    }
  }
}

void prepare_endpoints(const void *p)
{
  const usb_ep_desc_t *endpoint = (const usb_ep_desc_t *)p;
  esp_err_t err;

  // must be bulk for printer
  if ((endpoint->bmAttributes & USB_BM_ATTRIBUTES_XFERTYPE_MASK) != USB_BM_ATTRIBUTES_XFER_BULK) {
    ESP_LOGI("", "Not bulk endpoint: 0x%02x", endpoint->bmAttributes);
    return;
  }
  if (endpoint->bEndpointAddress & USB_B_ENDPOINT_ADDRESS_EP_DIR_MASK) {
    err = usb_host_transfer_alloc(endpoint->wMaxPacketSize, 0, &PrinterIn);
    if (err != ESP_OK) {
      PrinterIn = NULL;
      ESP_LOGI("", "usb_host_transfer_alloc In fail: %x", err);
      return;
    }
    PrinterIn->device_handle = Device_Handle;
    PrinterIn->bEndpointAddress = endpoint->bEndpointAddress;
    PrinterIn->callback = printer_transfer_cb;
    PrinterIn->context = NULL;
    PrinterIn->num_bytes = endpoint->wMaxPacketSize;
    esp_err_t err = usb_host_transfer_submit(PrinterIn);
    if (err != ESP_OK) {
      ESP_LOGI("", "usb_host_transfer_submit In fail: %x", err);
    }
  }
  else {
    err = usb_host_transfer_alloc(endpoint->wMaxPacketSize*PRINTER_OUT_BUFFERS, 0, &PrinterOut);
    if (err != ESP_OK) {
      PrinterOut = NULL;
      ESP_LOGI("", "usb_host_transfer_alloc Out fail: %x", err);
      return;
    }
    ESP_LOGI("", "Out data_buffer_size: %d", PrinterOut->data_buffer_size);
    PrinterOut->device_handle = Device_Handle;
    PrinterOut->bEndpointAddress = endpoint->bEndpointAddress;
    PrinterOut->callback = printer_transfer_cb;
    PrinterOut->context = NULL;
//    PrinterOut->flags |= USB_TRANSFER_FLAG_ZERO_PACK;
  }
}

void show_config_desc_full(const usb_config_desc_t *config_desc)
{
  // Full decode of config desc.
  const uint8_t *p = &config_desc->val[0];
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
          show_interface_desc(p);
          check_interface_desc_printer(p);
          break;
        case USB_B_DESCRIPTOR_TYPE_ENDPOINT:
          show_endpoint_desc(p);
          prepare_endpoints(p);
          break;
        default:
          ESP_LOGI("", "Unknown USB Descriptor Type: 0x%x", *p);
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
  Serial.begin(115200);
  Serial.setTimeout(0);
  usbh_setup(show_config_desc_full);
}

void loop()
{
  usbh_task();

  // ESP32 S2 Typewriter
  // Read line from serial monitor and write to printer.
  String aLine = Serial.readStringUntil('\n');  // Read line ending with newline

  if (aLine.length() > 0) {
    // readStringUntil removes the newline so add it back
    aLine.concat('\n');
    PrinterOut->num_bytes = aLine.length();
    memcpy(PrinterOut->data_buffer, aLine.c_str(), PrinterOut->num_bytes);
    esp_err_t err = usb_host_transfer_submit(PrinterOut);
    if (err != ESP_OK) {
      ESP_LOGI("", "usb_host_transfer_submit Out fail: %x", err);
    }
  }
}
