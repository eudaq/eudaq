/********************************************************************\

  Name:         musbstd.c
  Created by:   Konstantin Olchanski, Stefan Ritt

  Contents:     Midas USB access

  $Id$

\********************************************************************/

#include <stdio.h>
#include <assert.h>
#include <musbstd.h>

#ifdef _MSC_VER                 // Windows includes

#include <windows.h>
#include <conio.h>
#include <winioctl.h>

#include <setupapi.h>
#include <initguid.h>           /* Required for GUID definition */

// link with SetupAPI.Lib.
#pragma comment (lib, "setupapi.lib")

// disable "deprecated" warning
#pragma warning( disable: 4996)

// {CBEB3FB1-AE9F-471c-9016-9B6AC6DCD323}
DEFINE_GUID(GUID_CLASS_MSCB_BULK, 0xcbeb3fb1, 0xae9f, 0x471c, 0x90, 0x16, 0x9b, 0x6a, 0xc6, 0xdc, 0xd3, 0x23);

#elif defined(OS_DARWIN)

#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <fcntl.h>

#include <assert.h>
#include <mach/mach.h>
#include <IOKit/IOKitLib.h>
#include <IOKit/IOCFPlugIn.h>
#include <IOKit/usb/IOUSBLib.h>

#elif defined(OS_LINUX)         // Linux includes

#include <unistd.h>
#include <string.h>
#include <stdlib.h>

#endif

#ifdef HAVE_LIBUSB
#include <errno.h>
#include <usb.h>
#endif

#ifdef HAVE_LIBUSB10
#include <errno.h>
#include <libusb-1.0/libusb.h>
#endif

#if !defined(HAVE_LIBUSB) && !defined(HAVE_LIBUSB10)
#ifdef OS_DARWIN

IOReturn darwin_configure_device(MUSB_INTERFACE* musb)
{
   IOReturn status;
   io_iterator_t iter;
   io_service_t service;
   IOCFPlugInInterface **plugin;
   SInt32 score;
   IOUSBInterfaceInterface **uinterface;
   UInt8 numend;

   IOUSBDeviceInterface **device = (IOUSBDeviceInterface **)musb->device;

   status = (*device)->SetConfiguration(device, musb->usb_configuration);
   assert(status == kIOReturnSuccess);

   IOUSBFindInterfaceRequest request;

   request.bInterfaceClass = kIOUSBFindInterfaceDontCare;
   request.bInterfaceSubClass = kIOUSBFindInterfaceDontCare;
   request.bInterfaceProtocol = kIOUSBFindInterfaceDontCare;
   request.bAlternateSetting = kIOUSBFindInterfaceDontCare;
   
   status = (*device)->CreateInterfaceIterator(device, &request, &iter);
   assert(status == kIOReturnSuccess);
  
   while ((service = IOIteratorNext(iter))) {
      int i;
      status =
        IOCreatePlugInInterfaceForService(service, kIOUSBInterfaceUserClientTypeID,
                                          kIOCFPlugInInterfaceID, &plugin, &score);
      assert(status == kIOReturnSuccess);
      
      status =
        (*plugin)->QueryInterface(plugin, CFUUIDGetUUIDBytes(kIOUSBInterfaceInterfaceID),
                                  (void *) &uinterface);
      assert(status == kIOReturnSuccess);
      
      
      status = (*uinterface)->USBInterfaceOpen(uinterface);
      fprintf(stderr, "musb_open: USBInterfaceOpen status 0x%x\n", status);
      assert(status == kIOReturnSuccess);
      
      status = (*uinterface)->GetNumEndpoints(uinterface, &numend);
      assert(status == kIOReturnSuccess);
      
      fprintf(stderr, "musb_open: endpoints: %d\n", numend);
      
      for (i=1; i<=numend; i++) {
         status = (*uinterface)->GetPipeStatus(uinterface, i);
         fprintf(stderr, "musb_open: pipe %d status: 0x%x\n", i, status);
      
#if 0
         status = (*uinterface)->ClearPipeStall(uinterface, i);
         fprintf(stderr, "musb_open: pipe %d ClearPipeStall() status: 0x%x\n", i, status);
         status = (*uinterface)->ResetPipe(uinterface, i);
         fprintf(stderr, "musb_open: pipe %d ResetPipe() status: 0x%x\n", i, status);
         status = (*uinterface)->AbortPipe(uinterface, i);
         fprintf(stderr, "musb_open: pipe %d AbortPipe() status: 0x%x\n", i, status);
#endif
      }

      musb->interface = uinterface;
      return kIOReturnSuccess;
   }

   assert(!"Should never be reached!");
   return -1;
}
    
#endif

#endif

int musb_open(MUSB_INTERFACE **musb_interface, int vendor, int product, int instance, int configuration, int usbinterface)
{
#if defined(HAVE_LIBUSB)
   
   struct usb_bus *bus;
   struct usb_device *dev;
   int count = 0;
   
   usb_init();
   usb_find_busses();
   usb_find_devices();
   usb_set_debug(3);
   
   for (bus = usb_get_busses(); bus; bus = bus->next)
      for (dev = bus->devices; dev; dev = dev->next)
         if (dev->descriptor.idVendor == vendor && dev->descriptor.idProduct == product) {
            if (count == instance) {
               int status;
               usb_dev_handle *udev;

               udev = usb_open(dev);
               if (!udev) {
                  fprintf(stderr, "musb_open: usb_open() error\n");
                  return MUSB_ACCESS_ERROR;
               }

               status = usb_set_configuration(udev, configuration);
               if (status < 0) {
                  fprintf(stderr, "musb_open: usb_set_configuration() error %d (%s)\n", status,
                     strerror(-status));
                  fprintf(stderr,
                     "musb_open: Found USB device 0x%04x:0x%04x instance %d, but cannot initialize it: please check permissions on \"/proc/bus/usb/%s/%s\" and \"/dev/bus/usb/%s/%s\"\n",
                     vendor, product, instance, bus->dirname, dev->filename, bus->dirname, dev->filename);
                  return MUSB_ACCESS_ERROR;
               }

               /* see if we have write access */
               status = usb_claim_interface(udev, usbinterface);
               if (status < 0) {
                  fprintf(stderr, "musb_open: usb_claim_interface() error %d (%s)\n", status,
                     strerror(-status));

#ifdef _MSC_VER
                  fprintf(stderr,
                     "musb_open: Found USB device 0x%04x:0x%04x instance %d, but cannot initialize it:\nDevice is probably used by another program\n",
                     vendor, product, instance);
#else
                  fprintf(stderr,
                     "musb_open: Found USB device 0x%04x:0x%04x instance %d, but cannot initialize it: please check permissions on \"/proc/bus/usb/%s/%s\"\n",
                     vendor, product, instance, bus->dirname, dev->filename);
#endif

                  return MUSB_ACCESS_ERROR;
               }

               *musb_interface = (MUSB_INTERFACE*)calloc(1, sizeof(MUSB_INTERFACE));
               (*musb_interface)->dev = udev;
               (*musb_interface)->usb_configuration = configuration;
               (*musb_interface)->usb_interface     = usbinterface;
               return MUSB_SUCCESS;
            }

            count++;
         }
   
   return MUSB_NOT_FOUND;
   
#elif defined(HAVE_LIBUSB10)
   
   static int first_call = 1;

   libusb_device **dev_list;
   libusb_device_handle *dev;
   struct libusb_device_descriptor desc;
   
   int status, i, n;
   int count = 0;
   
   if (first_call) {
      first_call = 0;
      libusb_init(NULL);
      // libusb_set_debug(NULL, 3);
   }
      
   n = libusb_get_device_list(NULL, &dev_list);
   
   for (i=0 ; i<n ; i++) {
      status = libusb_get_device_descriptor(dev_list[i], &desc);
      if (desc.idVendor == vendor && desc.idProduct == product) {
         if (count == instance) {
            status = libusb_open(dev_list[i], &dev);
            if (status < 0) {
               fprintf(stderr, "musb_open: libusb_open() error %d\n", status);
               return MUSB_ACCESS_ERROR;
            }

            status = libusb_set_configuration(dev, configuration);
            if (status < 0) {
               fprintf(stderr, "musb_open: usb_set_configuration() error %d\n", status);
               fprintf(stderr,
                       "musb_open: Found USB device 0x%04x:0x%04x instance %d, but cannot initialize it: please check permissions on \"/proc/bus/usb/%d/%d\" and \"/dev/bus/usb/%d/%d\"\n",
                       vendor, product, instance, libusb_get_bus_number(dev_list[i]), libusb_get_device_address(dev_list[i]), libusb_get_bus_number(dev_list[i]), libusb_get_device_address(dev_list[i]));
               return MUSB_ACCESS_ERROR;
            }
            
            /* see if we have write access */
            status = libusb_claim_interface(dev, usbinterface);
            if (status < 0) {
               fprintf(stderr, "musb_open: libusb_claim_interface() error %d\n", status);
               
#ifdef _MSC_VER
               fprintf(stderr,
                       "musb_open: Found USB device 0x%04x:0x%04x instance %d, but cannot initialize it:\nDevice is probably used by another program\n",
                       vendor, product, instance);
#else
               fprintf(stderr,
                       "musb_open: Found USB device 0x%04x:0x%04x instance %d, but cannot initialize it: please check permissions on \"/proc/bus/usb/%d/%d\"\n",
                       vendor, product, instance, libusb_get_bus_number(dev_list[i]), libusb_get_device_address(dev_list[i]));
#endif
               
               return MUSB_ACCESS_ERROR;
            }

            *musb_interface = (MUSB_INTERFACE*)calloc(1, sizeof(MUSB_INTERFACE));
            (*musb_interface)->dev = dev;
            (*musb_interface)->usb_configuration = configuration;
            (*musb_interface)->usb_interface     = usbinterface;
            return MUSB_SUCCESS;

         }
         count++;
      }
   }
   
   libusb_free_device_list(dev_list, 1);
   
   return MUSB_NOT_FOUND;
     
#elif defined(OS_DARWIN)
   
   kern_return_t status;
   io_iterator_t iter;
   io_service_t service;
   IOCFPlugInInterface **plugin;
   SInt32 score;
   IOUSBDeviceInterface **device;
   UInt16 xvendor, xproduct;
   int count = 0;

   *musb_interface = calloc(1, sizeof(MUSB_INTERFACE));
   
   status = IORegistryCreateIterator(kIOMasterPortDefault, kIOUSBPlane, kIORegistryIterateRecursively, &iter);
   assert(status == kIOReturnSuccess);

   while ((service = IOIteratorNext(iter))) {
      status =
          IOCreatePlugInInterfaceForService(service, kIOUSBDeviceUserClientTypeID, kIOCFPlugInInterfaceID,
                                            &plugin, &score);
      assert(status == kIOReturnSuccess);

      status = IOObjectRelease(service);
      assert(status == kIOReturnSuccess);

      status =
          (*plugin)->QueryInterface(plugin, CFUUIDGetUUIDBytes(kIOUSBDeviceInterfaceID), (void *) &device);
      assert(status == kIOReturnSuccess);

      status = (*plugin)->Release(plugin);

      status = (*device)->GetDeviceVendor(device, &xvendor);
      assert(status == kIOReturnSuccess);
      status = (*device)->GetDeviceProduct(device, &xproduct);
      assert(status == kIOReturnSuccess);

      //fprintf(stderr, "musb_open: Found USB device: vendor 0x%04x, product 0x%04x\n", xvendor, xproduct);

      if (xvendor == vendor && xproduct == product) {
         if (count == instance) {

            fprintf(stderr, "musb_open: Found USB device: vendor 0x%04x, product 0x%04x, instance %d\n", xvendor, xproduct, instance);

            status = (*device)->USBDeviceOpen(device);
            fprintf(stderr, "musb_open: USBDeviceOpen status 0x%x\n", status);

            assert(status == kIOReturnSuccess);

            (*musb_interface)->usb_configuration = configuration;
            (*musb_interface)->usb_interface     = usbinterface;
            (*musb_interface)->device = (void*)device;
            (*musb_interface)->interface = NULL;

            status = darwin_configure_device(*musb_interface);

            if (status == kIOReturnSuccess)
               return MUSB_SUCCESS;

            fprintf(stderr, "musb_open: USB device exists, but configuration fails!");
            return MUSB_NOT_FOUND;
         }

         count++;
      }

      (*device)->Release(device);
   }

   return MUSB_NOT_FOUND;
#elif defined(_MSC_VER)
   GUID guid;
   HDEVINFO hDevInfoList;
   SP_DEVICE_INTERFACE_DATA deviceInfoData;
   PSP_DEVICE_INTERFACE_DETAIL_DATA functionClassDeviceData;
   ULONG predictedLength, requiredLength;
   int status;
   char device_name[256], str[256];

   *musb_interface = (MUSB_INTERFACE *)calloc(1, sizeof(MUSB_INTERFACE));

   guid = GUID_CLASS_MSCB_BULK;

   // Retrieve device list for GUID that has been specified.
   hDevInfoList = SetupDiGetClassDevs(&guid, NULL, NULL, (DIGCF_PRESENT | DIGCF_DEVICEINTERFACE));

   status = FALSE;
   if (hDevInfoList != NULL) {

      // Clear data structure
      memset(&deviceInfoData, 0, sizeof(deviceInfoData));
      deviceInfoData.cbSize = sizeof(SP_DEVICE_INTERFACE_DATA);

      // retrieves a context structure for a device interface of a device information set.
      if (SetupDiEnumDeviceInterfaces(hDevInfoList, 0, &guid, instance, &deviceInfoData)) {
         // Must get the detailed information in two steps
         // First get the length of the detailed information and allocate the buffer
         // retrieves detailed information about a specified device interface.
         functionClassDeviceData = NULL;

         predictedLength = requiredLength = 0;

         SetupDiGetDeviceInterfaceDetail(hDevInfoList, &deviceInfoData, NULL,   // Not yet allocated
                                         0,     // Set output buffer length to zero
                                         &requiredLength,       // Find out memory requirement
                                         NULL);

         predictedLength = requiredLength;
         functionClassDeviceData = (PSP_DEVICE_INTERFACE_DETAIL_DATA) malloc(predictedLength);
         functionClassDeviceData->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA);

         // Second, get the detailed information
         if (SetupDiGetDeviceInterfaceDetail(hDevInfoList,
                                             &deviceInfoData, functionClassDeviceData,
                                             predictedLength, &requiredLength, NULL)) {

            // Save the device name for subsequent pipe open calls
            strcpy(device_name, functionClassDeviceData->DevicePath);
            free(functionClassDeviceData);

            // Signal device found
            status = TRUE;
         } else
            free(functionClassDeviceData);
      }
   }
   // SetupDiDestroyDeviceInfoList() destroys a device information set
   // and frees all associated memory.
   SetupDiDestroyDeviceInfoList(hDevInfoList);

   if (status) {

      // Get the read handle
      sprintf(str, "%s\\PIPE00", device_name);
      (*musb_interface)->rhandle = CreateFile(str,
                                    GENERIC_WRITE | GENERIC_READ,
                                    FILE_SHARE_WRITE | FILE_SHARE_READ, NULL,
                                    OPEN_EXISTING, FILE_FLAG_OVERLAPPED, NULL);

      if ((*musb_interface)->rhandle == INVALID_HANDLE_VALUE)
         return MUSB_ACCESS_ERROR;

      // Get the write handle
      sprintf(str, "%s\\PIPE01", device_name);
      (*musb_interface)->whandle = CreateFile(str,
                                    GENERIC_WRITE | GENERIC_READ,
                                    FILE_SHARE_WRITE | FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);

      if ((*musb_interface)->whandle == INVALID_HANDLE_VALUE)
         return MUSB_ACCESS_ERROR;

      return MUSB_SUCCESS;
   } 
     
   return MUSB_NOT_FOUND;
#endif
}

int musb_set_altinterface(MUSB_INTERFACE *musb_interface, int index)
{
#if defined (HAVE_LIBUSB)
   int status;

   status = usb_set_altinterface(musb_interface->dev, index);
   if (status < 0)
      fprintf(stderr, "musb_set_altinterface: usb_set_altinterface() error %d\n", status);

   return status;
#else
   return -1;
#endif
}

int musb_close(MUSB_INTERFACE *musb_interface)
{
#if defined(HAVE_LIBUSB)

   int status;
   status = usb_release_interface(musb_interface->dev, musb_interface->usb_interface);
   if (status < 0)
      fprintf(stderr, "musb_close: usb_release_interface() error %d\n", status);
   
#ifdef OS_LINUX   // linux wants a reset, otherwise the device cannot be accessed next time
   musb_reset(musb_interface);
#endif

   status = usb_close(musb_interface->dev);
   if (status < 0)
      fprintf(stderr, "musb_close: usb_close() error %d\n", status);
   
#elif defined(HAVE_LIBUSB10)   

   int status;
   status = libusb_release_interface(musb_interface->dev, musb_interface->usb_interface);
   if (status < 0)
      fprintf(stderr, "musb_close: libusb_release_interface() error %d\n", status);
   
#ifdef OS_LINUX   // linux wants a reset, otherwise the device cannot be accessed next time
   musb_reset(musb_interface);
#endif
   
   libusb_close(musb_interface->dev);

#elif defined(OS_DARWIN)

   IOReturn status;
   IOUSBInterfaceInterface **interface = (IOUSBInterfaceInterface **)musb_interface->interface;

   status = (*interface)->USBInterfaceClose(interface);
   if (status != kIOReturnSuccess)
      fprintf(stderr, "musb_close: USBInterfaceClose() status %d 0x%x\n", status, status);

   status = (*interface)->Release(interface);
   if (status != kIOReturnSuccess)
      fprintf(stderr, "musb_close: USB Interface Release() status %d 0x%x\n", status, status);

   IOUSBDeviceInterface **device = (IOUSBDeviceInterface**)musb_interface->device;
   status = (*device)->USBDeviceClose(device);
   if (status != kIOReturnSuccess)
      fprintf(stderr, "musb_close: USBDeviceClose() status %d 0x%x\n", status, status);

   status = (*device)->Release(device);
   if (status != kIOReturnSuccess)
      fprintf(stderr, "musb_close: USB Device Release() status %d 0x%x\n", status, status);

#elif defined(_MSC_VER)

   CloseHandle(musb_interface->rhandle);
   CloseHandle(musb_interface->whandle);

#else
   assert(!"musb_close() is not implemented");
#endif

   /* free memory allocated in musb_open() */
   free(musb_interface);
   return 0;
}

int musb_write(MUSB_INTERFACE *musb_interface, int endpoint, const void *buf, int count, int timeout)
{
   int n_written;

#if defined(HAVE_LIBUSB)
   n_written = usb_bulk_write(musb_interface->dev, endpoint, (char*)buf, count, timeout);
   if (n_written != count) {
      fprintf(stderr, "musb_write: requested %d, wrote %d, errno %d (%s)\n", count, n_written, errno, strerror(errno));
   }
#elif defined(HAVE_LIBUSB10)
   int status = libusb_bulk_transfer(musb_interface->dev, endpoint, (unsigned char*)buf, count, &n_written, timeout);
   if (n_written != count) {
      fprintf(stderr, "musb_write: requested %d, wrote %d, errno %d (%s)\n", count, n_written, status, strerror(status));
   }
#elif defined(OS_DARWIN)
   IOReturn status;
   IOUSBInterfaceInterface182 **interface = (IOUSBInterfaceInterface182 **)musb_interface->interface;
   status = (*interface)->WritePipeTO(interface, endpoint, buf, count, 0, timeout);
   if (status != 0) {
      fprintf(stderr, "musb_write: WritePipe() status %d 0x%x\n", status, status);
      return -1;
   }
   n_written = count;
#elif defined(_MSC_VER)
   WriteFile(musb_interface->whandle, buf, count, &n_written, NULL);
#endif

   //fprintf(stderr, "musb_write(ep %d, %d bytes) (%s) returns %d\n", endpoint, count, buf, n_written); 

   return n_written;
}

int musb_read(MUSB_INTERFACE *musb_interface, int endpoint, void *buf, int count, int timeout)
{
   int n_read = 0;

#if defined(HAVE_LIBUSB)

   n_read = usb_bulk_read(musb_interface->dev, endpoint | 0x80, (char*)buf, count, timeout);
   /* errors should be handled in upper layer ....
   if (n_read <= 0) {
      fprintf(stderr, "musb_read: requested %d, read %d, errno %d (%s)\n", count, n_read, errno, strerror(errno));
   }
   */

#elif defined(HAVE_LIBUSB10)
   
   libusb_bulk_transfer(musb_interface->dev, endpoint | 0x80, (unsigned char*)buf, count, &n_read, timeout);
   /* errors should be handled in upper layer ....
    if (n_read <= 0) {
    fprintf(stderr, "musb_read: requested %d, read %d, errno %d (%s)\n", count, n_read, status, strerror(status));
    }
    */
   
#elif defined(OS_DARWIN)

   UInt32 xcount = count;
   IOReturn status;
   IOUSBInterfaceInterface182 **interface = (IOUSBInterfaceInterface182 **)musb_interface->interface;

   status = (*interface)->ReadPipeTO(interface, endpoint, buf, &xcount, 0, timeout);
   if (status != kIOReturnSuccess) {
      fprintf(stderr, "musb_read: requested %d, read %d, ReadPipe() status %d 0x%x (%s)\n", count, n_read, status, status, strerror(status));
      return -1;
   }

   n_read = xcount;

#elif defined(_MSC_VER)

   OVERLAPPED overlapped;
   int status;

   memset(&overlapped, 0, sizeof(overlapped));
   overlapped.hEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
   n_read = 0;

   status = ReadFile(musb_interface->rhandle, buf, count, &n_read, &overlapped);
   
   if (!status) {

      status = GetLastError();
      if (status != ERROR_IO_PENDING)
         return 0;

      /* wait for completion with timeout */
      status = WaitForSingleObject(overlapped.hEvent, timeout);
      if (status == WAIT_TIMEOUT)
         CancelIo(musb_interface->rhandle);
      else
         GetOverlappedResult(musb_interface->rhandle, &overlapped, &n_read, FALSE);
   }

   CloseHandle(overlapped.hEvent);

#endif

   //fprintf(stderr, "musb_read(ep %d, %d bytes) returns %d (%s)\n", endpoint, count, n_read, buf); 

   return n_read;
}

int musb_reset(MUSB_INTERFACE *musb_interface)
{
#if defined(HAVE_LIBUSB)

   /* Causes re-enumeration: After calling usb_reset, the device will need 
      to re-enumerate and thusly, requires you to find the new device and 
      open a new handle. The handle used to call usb_reset will no longer work */

   int status;
   status = usb_reset(musb_interface->dev);
   if (status < 0)
      fprintf(stderr, "musb_reset: usb_reset() status %d\n", status);

#elif defined(HAVE_LIBUSB10)
   
   int status;
   status = libusb_reset_device(musb_interface->dev);
   if (status < 0)
      fprintf(stderr, "musb_reset: usb_reset() status %d\n", status);
   
#elif defined(OS_DARWIN)

   IOReturn status;
   IOUSBDeviceInterface **device = (IOUSBDeviceInterface**)musb_interface->device;

   status = (*device)->ResetDevice(device);
   fprintf(stderr, "musb_reset: ResetDevice() status 0x%x\n", status);

   status = darwin_configure_device(musb_interface);
   assert(status == kIOReturnSuccess);

#elif defined(_MSC_VER)

#define IOCTL_BULKUSB_RESET_DEVICE          CTL_CODE(FILE_DEVICE_UNKNOWN,     \
                                                     1,                       \
                                                     METHOD_BUFFERED,         \
                                                     FILE_ANY_ACCESS)
#define IOCTL_BULKUSB_RESET_PIPE            CTL_CODE(FILE_DEVICE_UNKNOWN,     \
                                                     2,                       \
                                                     METHOD_BUFFERED,         \
                                                     FILE_ANY_ACCESS)

   int status, n_bytes;

   status =  DeviceIoControl(musb_interface->rhandle,
                  IOCTL_BULKUSB_RESET_DEVICE,
                  NULL, 0, NULL, 0, &n_bytes, NULL);
   status =  DeviceIoControl(musb_interface->whandle,
                  IOCTL_BULKUSB_RESET_DEVICE,
                  NULL, 0, NULL, 0, &n_bytes, NULL);
   status =  DeviceIoControl(musb_interface->rhandle,
                  IOCTL_BULKUSB_RESET_PIPE,
                  NULL, 0, NULL, 0, &n_bytes, NULL);
   status =  DeviceIoControl(musb_interface->whandle,
                  IOCTL_BULKUSB_RESET_PIPE,
                  NULL, 0, NULL, 0, &n_bytes, NULL);
   return status;

#endif
   return 0;
}

int musb_get_device(MUSB_INTERFACE *usb_interface)
{
#ifdef HAVE_LIBUSB
   struct usb_device_descriptor d;
   usb_get_descriptor(usb_interface->dev, USB_DT_DEVICE, 0, &d, sizeof(d));
   return d.bcdDevice;
#elif HAVE_LIBUSB10
   struct libusb_device_descriptor d;
   libusb_get_descriptor(usb_interface->dev, LIBUSB_DT_DEVICE, 0, (unsigned char *)&d, sizeof(d));
   return d.bcdDevice;
#else
   return 0;
#endif
}

/* end */
