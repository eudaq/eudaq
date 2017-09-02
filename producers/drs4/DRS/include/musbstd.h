/********************************************************************\

  Name:         musbstd.h
  Created by:   Konstantin Olchanski, Stefan Ritt

  Contents:     Midas USB access

  $Id$

\********************************************************************/

#ifndef MUSBSTD_H
#define MUSBSTD_H

#if defined(HAVE_LIBUSB)

#include <usb.h>

typedef struct {
   usb_dev_handle *dev;
   int usb_configuration;
   int usb_interface;
   int usb_type;
} MUSB_INTERFACE;

#elif defined(HAVE_LIBUSB10)

#include <libusb-1.0/libusb.h>

typedef struct {
   libusb_device_handle *dev;
   int usb_configuration;
   int usb_interface;
   int usb_type;
} MUSB_INTERFACE;

#elif defined(_MSC_VER)

#include <windows.h>

typedef struct {
   HANDLE rhandle;
   HANDLE whandle;
   int usb_type;
} MUSB_INTERFACE;

#elif defined(OS_DARWIN)

typedef struct {
   void *device;
   void *interface;
   int usb_configuration;
   int usb_interface;
   int usb_type;
} MUSB_INTERFACE;

#else
#error Do not know how to access USB devices
#endif

/*---- status codes ------------------------------------------------*/

#define MUSB_SUCCESS                  1
#define MUSB_NOT_FOUND                2
#define MUSB_INVALID_PARAM            3
#define MUSB_NO_MEM                   4
#define MUSB_ACCESS_ERROR             5

/* make functions callable from a C++ program */
#ifdef __cplusplus
extern "C" {
#endif

/* make functions under WinNT dll exportable */
#ifndef EXPRT
#if defined(_MSC_VER) && defined(_USRDLL)
#define EXPRT __declspec(dllexport)
#else
#define EXPRT
#endif
#endif

int EXPRT musb_open(MUSB_INTERFACE **musb_interface, int vendor, int product, int instance, int configuration, int usbinterface);
int EXPRT musb_close(MUSB_INTERFACE *musb_interface);
int EXPRT musb_write(MUSB_INTERFACE *musb_interface,int endpoint,const void *buf,int count,int timeout_ms);
int EXPRT musb_read(MUSB_INTERFACE *musb_interface,int endpoint,void *buf,int count,int timeout_ms);
int EXPRT musb_reset(MUSB_INTERFACE *musb_interface);
int EXPRT musb_set_altinterface(MUSB_INTERFACE *musb_interface, int index);
int EXPRT musb_get_device(MUSB_INTERFACE *musb_interface);

#ifdef __cplusplus
}
#endif

#endif // MUSBSTD_H
