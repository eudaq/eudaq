// WinUSBConsoleApplication.cpp : De// WinUSBConsoleApplication.cpp : Defines the entry point for the console application.
//

#ifdef _WIN32
#define _WINSOCKAPI_	// stops windows.h including winsock.h

//#include "stdafx.h"
#include <Windows.h>
#include <setupapi.h>
#include <Dbt.h>		//Need this for definitions of WM_DEVICECHANGE messages
#include <Winusb.h>
#else
#include <libusb.h>   //libusb-1.0/
#endif
#include <string>
#include <iostream>
#include <chrono>
#include <algorithm>
#include <cctype>
#include <string>
#include <thread>
#include <iostream>
#include <sstream>
#include <mutex>
#include <vector>

#include "FERS_LL.h"
//#include "FERSlib.h"

#ifdef WIN32
#pragma comment(lib, "setupapi.lib")
#pragma comment(lib, "winusb.lib")
#endif

// *********************************************************************************************************
// Functions related to USB driver and low level communication. This part is C++.
// *********************************************************************************************************
using namespace std;

#define TIMEOUT  1000

#ifdef WIN32

//Modify this value to match the VID and PID in your USB device descriptor.
//Use the formatting: "Vid_xxxx&Pid_xxxx" where xxxx is a 16-bit hexadecimal number.
#define MY_DEVICE_ID  L"Vid_04d8&Pid_0053"		   //Change this number (along with the corresponding VID/PID in the
												   //microcontroller firmware, and in the driver installation .INF
												   //file) before moving the design into production.


class USBDEV {
private:

	//----------------------------------------------------------------
	//Application variables that need to have wide scope.
	//----------------------------------------------------------------
	volatile BOOL AttachedState = FALSE;							//Need to keep track of the USB device attachment status for proper plug and play operation.
	PSP_DEVICE_INTERFACE_DETAIL_DATA DetailedInterfaceDataStructure = new SP_DEVICE_INTERFACE_DETAIL_DATA;	//Global
	HANDLE MyDeviceHandle = INVALID_HANDLE_VALUE;		//First need to get the Device handle
	WINUSB_INTERFACE_HANDLE MyWinUSBInterfaceHandle;	//And then can call WinUsb_Initialize() to get the interface handle
														//which is needed for doing other operations with the device (like
														//reading and writing to the USB device).


	BOOL	CheckIfPresentAndGetUSBDevicePath(DWORD InterfaceIndex);

	GUID InterfaceClassGuid = { 0xa5dcbf10, 0x6530, 0x11d2, 0x90, 0x1F, 0x00, 0xC0, 0x4F, 0xB9, 0x51, 0xED }; //Globally Unique Identifier (GUID) for USB peripheral devices
	std::mutex g_interface_mutex;
	// Mutex principale per proteggere le operazioni concorrenti
	std::mutex main_mutex;
	

	// Flag per indicare se lo stream e' attualmente abilitato
	bool streaming_active = false;

	// Buffer per memorizzare dati "rimasti" nel FIFO quando lo stream viene disabilitato
	std::vector<char> leftover_data;

private:
	// Funzione di utility per svuotare completamente la FIFO su ep 0x82
	int drain_fifo(std::vector<char>& outData);
	int stream_enable2(bool enable);
public:

	bool IsOpen = FALSE;

	int open_connection(int index);
	void close_connection();
	int set_service_reg(uint32_t address, uint32_t data);

	int write_mem(uint32_t address, uint32_t length, char* data);
	int read_mem(uint32_t address, uint32_t length, char* data);

	int write_reg(uint32_t address, uint32_t data);

	int read_reg(uint32_t address, uint32_t* data);

	int stream_enable(bool enable);

	int read_pipe(char* buff, int size, int* nb);

	int reset_leftover_data() {
		leftover_data.clear();
		leftover_data.shrink_to_fit();
		return 0;
	}

};
#else
	class USBDEV {
		private:

			libusb_device_handle *dev_handle = nullptr;
			static libusb_context *ctx; //a libusb session
			static unsigned int occurency;

			// Mutex principale per proteggere le operazioni concorrenti
			std::mutex main_mutex;

			// Flag per indicare se lo stream e' attualmente abilitato
			bool streaming_active = false;

			// Buffer per memorizzare dati "rimasti" nel FIFO quando lo stream viene disabilitato
			std::vector<char> leftover_data;

			bool	CheckIfPresentAndGetUSBDevicePath(int InterfaceIndex, libusb_device_handle **dev_handle);

			int USBSend(unsigned char* outBuffer, unsigned char* inBuffer, int outsize, int insize);

			// Funzione di utility per svuotare completamente la FIFO su ep 0x82
			int drain_fifo(std::vector<char>& outData);
			int stream_enable2(bool enable);

		public:

			USBDEV();

			bool IsOpen = 0;

			int open_connection(int index);

			void close_connection();

			int set_service_reg(uint32_t address, uint32_t data);

			int write_mem(uint32_t address, uint32_t length,  char *data);

			int read_mem(uint32_t address, uint32_t length, char *data);

			int write_reg(uint32_t address, uint32_t data);

			int read_reg(uint32_t address, uint32_t *data);
			int stream_enable(bool enable);
			int read_pipe(char *buff, int size, int *nb);

			int reset_leftover_data() {
				leftover_data.clear();
				leftover_data.shrink_to_fit();
				return 0;
			}
};
#endif
