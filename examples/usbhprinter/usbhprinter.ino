#include <usb/usb_host.h>
#include "show_desc.hpp"

const TickType_t HOST_EVENT_TIMEOUT = 1;
const TickType_t CLIENT_EVENT_TIMEOUT = 1;

usb_host_client_handle_t Client_Handle;
usb_device_handle_t Device_Handle;
bool isAPrinter = false;
bool isABiDirPrinter = false;

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
      isABiDirPrinter = (intf->bInterfaceProtocol == 2);
      if (isABiDirPrinter) {
        if (intf->bNumEndpoints < 2) {
          isAPrinter = false;
          return;
        }
      }
      else {
        if (intf->bNumEndpoints < 1) {
          isAPrinter = false;
          return;
        }
      }
      isAPrinter = true;
      ESP_LOGI("", "Claiming a %s-directional printer!", (isABiDirPrinter)?"bi":"uni");
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
      // USB Printer Class Specification 1.1
      if ((dev_desc->bDeviceClass == USB_CLASS_PRINTER) &&
          (dev_desc->bDeviceSubClass == 1))
      {
        /* Protocol
         * 00 Reserved, undefined.
         * 01 Unidirectional interface.
         * 02 Bi-directional interface.
         * 03 1284.4 compatible bi-directional interface.
         * 04-FEh Reserved for future use.
         * FFh Vendor-specific printers do not use class-specific protocols.
         */
        if ((dev_desc->bDeviceProtocol == 1) || (dev_desc->bDeviceProtocol == 2)) {
          // No idea how to support 1284.4 so ignore it.
          isAPrinter = true;
          isABiDirPrinter = (dev_desc->bDeviceProtocol == 2);
          ESP_LOGI("", "Found a %s-directional printer!", (isABiDirPrinter)?"bi":"uni");
        }
      }

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
  static bool done = false;

  esp_err_t err = usb_host_lib_handle_events(HOST_EVENT_TIMEOUT, &event_flags);
  if (ESP_ERR_TIMEOUT != err) {
    ESP_LOGI("", "usb_host_list_handle_events: %x flags: %x", err, event_flags);
  }

  err = usb_host_client_handle_events(Client_Handle, CLIENT_EVENT_TIMEOUT);
  if (ESP_ERR_TIMEOUT != err) {
    ESP_LOGI("", "usb_host_client_handle_events: %x", err);
  }

  if (!done && (PrinterOut != NULL)) {
    ESP_LOGI("", "Send to printer");
    const char HELLO[]="Hello from ESP32 S2\n";
    memcpy(PrinterOut->data_buffer, HELLO, sizeof(HELLO)-1);
    PrinterOut->num_bytes = sizeof(HELLO)-1; // Exclude the terminating '\0'
    err = usb_host_transfer_submit(PrinterOut);
    if (err != ESP_OK) {
      ESP_LOGI("", "usb_host_transfer_submit Out fail: %x", err);
    }
    done = true;
  }
}
