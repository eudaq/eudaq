// WinUSBConsoleApplication.cpp : Defines the entry point for the console application.
//

#include "FERS_LLusb.h"
//#include "FERSlib.h"
//#include <algorithm>   // per std::min
#include <cstring>     // per memcpy
#include <cstdio>      // per sprintf
#ifdef _WIN32
#pragma comment(lib, "setupapi.lib")
#pragma comment(lib, "winusb.lib")
#endif

// *********************************************************************************************************
// Functions related to USB driver and low level communication. This part is C++.
// *********************************************************************************************************
using namespace std;

int streamMutex_initialized;

int8_t usbIdx[FERSLIB_MAX_NBRD] = {};
//static mutex_t USBStream_Mutex = NULL;

#define TIMEOUT  1000

#ifdef WIN32

//Modify this value to match the VID and PID in your USB device descriptor.
//Use the formatting: "Vid_xxxx&Pid_xxxx" where xxxx is a 16-bit hexadecimal number.
#define MY_DEVICE_ID  L"Vid_04d8&Pid_0053"		   //Change this number (along with the corresponding VID/PID in the
												   //microcontroller firmware, and in the driver installation .INF
												   //file) before moving the design into production.


BOOL	USBDEV::CheckIfPresentAndGetUSBDevicePath(DWORD InterfaceIndex) {

	GUID InterfaceClassGuid = { 0x58D07210, 0x27C1, 0x11DD, 0xBD, 0x0B, 0x08, 0x00, 0x20, 0x0C, 0x9A, 0x66 };

	HDEVINFO DeviceInfoTable = INVALID_HANDLE_VALUE;
	PSP_DEVICE_INTERFACE_DATA InterfaceDataStructure = new SP_DEVICE_INTERFACE_DATA;
	//		PSP_DEVICE_INTERFACE_DETAIL_DATA DetailedInterfaceDataStructure = new SP_DEVICE_INTERFACE_DETAIL_DATA;	//Global
	SP_DEVINFO_DATA DevInfoData;

	//DWORD InterfaceIndex = 0;
	DWORD dwRegType;
	DWORD dwRegSize;
	DWORD StructureSize = 0;
	PBYTE PropertyValueBuffer;
	bool MatchFound = false;
	DWORD ErrorStatus;
	DWORD LoopCounter = 0;

	wstring DeviceIDToFind = MY_DEVICE_ID;

	//First populate a list of plugged in devices (by specifying "DIGCF_PRESENT"), which are of the specified class GUID.
	DeviceInfoTable = SetupDiGetClassDevs(&InterfaceClassGuid, NULL, NULL, DIGCF_PRESENT | DIGCF_DEVICEINTERFACE);

	//Now look through the list we just populated.  We are trying to see if any of them match our device.
	while (true)
	{
		InterfaceDataStructure->cbSize = sizeof(SP_DEVICE_INTERFACE_DATA);
		if (SetupDiEnumDeviceInterfaces(DeviceInfoTable, NULL, &InterfaceClassGuid, InterfaceIndex, InterfaceDataStructure))
		{
			ErrorStatus = GetLastError();
			if (ErrorStatus == ERROR_NO_MORE_ITEMS)	//Did we reach the end of the list of matching devices in the DeviceInfoTable?
			{	//Cound not find the device.  Must not have been attached.
				SetupDiDestroyDeviceInfoList(DeviceInfoTable);	//Clean up the old structure we no longer need.
				return FALSE;
			}
		} else	//Else some other kind of unknown error ocurred...
		{
			ErrorStatus = GetLastError();
			SetupDiDestroyDeviceInfoList(DeviceInfoTable);	//Clean up the old structure we no longer need.
			return FALSE;
		}

		//Now retrieve the hardware ID from the registry.  The hardware ID contains the VID and PID, which we will then
		//check to see if it is the correct device or not.

		//Initialize an appropriate SP_DEVINFO_DATA structure.  We need this structure for SetupDiGetDeviceRegistryProperty().
		DevInfoData.cbSize = sizeof(SP_DEVINFO_DATA);
		SetupDiEnumDeviceInfo(DeviceInfoTable, InterfaceIndex, &DevInfoData);

		//First query for the size of the hardware ID, so we can know how big a buffer to allocate for the data.
		SetupDiGetDeviceRegistryProperty(DeviceInfoTable, &DevInfoData, SPDRP_HARDWAREID, &dwRegType, NULL, 0, &dwRegSize);

		//Allocate a buffer for the hardware ID.
		PropertyValueBuffer = (BYTE*)malloc(dwRegSize);
		if (PropertyValueBuffer == NULL)	//if null, error, couldn't allocate enough memory
		{	//Can't really recover from this situation, just exit instead.
			SetupDiDestroyDeviceInfoList(DeviceInfoTable);	//Clean up the old structure we no longer need.
			return FALSE;
		}

		//Retrieve the hardware IDs for the current device we are looking at.  PropertyValueBuffer gets filled with a
		//REG_MULTI_SZ (array of null terminated strings).  To find a device, we only care about the very first string in the
		//buffer, which will be the "device ID".  The device ID is a string which contains the VID and PID, in the example
		//format "Vid_04d8&Pid_003f".
		SetupDiGetDeviceRegistryProperty(DeviceInfoTable, &DevInfoData, SPDRP_HARDWAREID, &dwRegType, PropertyValueBuffer, dwRegSize, NULL);

		//Now check if the first string in the hardware ID matches the device ID of my USB device.
#ifdef UNICODE
		wstring* DeviceIDFromRegistry = new wstring((wchar_t*)PropertyValueBuffer);
#else
		string DeviceIDFromRegistry = new string((char*)PropertyValueBuffer);
#endif

		free(PropertyValueBuffer);		//No longer need the PropertyValueBuffer, free the memory to prevent potential memory leaks

		//Convert both strings to lower case.  This makes the code more robust/portable accross OS Versions
		std::transform(DeviceIDFromRegistry->begin(), DeviceIDFromRegistry->end(), DeviceIDFromRegistry->begin(),
			[](unsigned char c) { return std::tolower(c); });
		std::transform(DeviceIDToFind.begin(), DeviceIDToFind.end(), DeviceIDToFind.begin(),
			[](unsigned char c) { return std::tolower(c); });

		//DeviceIDFromRegistry = DeviceIDFromRegistry ->ToLowerInvariant();
		//DeviceIDToFind = DeviceIDToFind->ToLowerInvariant();
		//Now check if the hardware ID we are looking at contains the correct VID/PID
		if (DeviceIDFromRegistry->find(DeviceIDToFind) != std::wstring::npos) {
			MatchFound = true;
		}

		if (MatchFound == true)
		{
			//Device must have been found.  Open WinUSB interface handle now.  In order to do this, we will need the actual device path first.
			//We can get the path by calling SetupDiGetDeviceInterfaceDetail(), however, we have to call this function twice:  The first
			//time to get the size of the required structure/buffer to hold the detailed interface data, then a second time to actually
			//get the structure (after we have allocated enough memory for the structure.)
			DetailedInterfaceDataStructure->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA);
			//First call populates "StructureSize" with the correct value
			SetupDiGetDeviceInterfaceDetail(DeviceInfoTable, InterfaceDataStructure, NULL, NULL, &StructureSize, NULL);
			DetailedInterfaceDataStructure = (PSP_DEVICE_INTERFACE_DETAIL_DATA)(malloc(StructureSize));		//Allocate enough memory
			if (DetailedInterfaceDataStructure == NULL)	//if null, error, couldn't allocate enough memory
			{	//Can't really recover from this situation, just exit instead.
				SetupDiDestroyDeviceInfoList(DeviceInfoTable);	//Clean up the old structure we no longer need.
				return FALSE;
			}
			DetailedInterfaceDataStructure->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA);
			//Now call SetupDiGetDeviceInterfaceDetail() a second time to receive the goods.
			SetupDiGetDeviceInterfaceDetail(DeviceInfoTable, InterfaceDataStructure, DetailedInterfaceDataStructure, StructureSize, NULL, NULL);

			//We now have the proper device path, and we can finally open a device handle to the device.
			//WinUSB requires the device handle to be opened with the FILE_FLAG_OVERLAPPED attribute.
			SetupDiDestroyDeviceInfoList(DeviceInfoTable);	//Clean up the old structure we no longer need.
			return TRUE;
		}
		InterfaceIndex++;
		//Keep looping until we either find a device with matching VID and PID, or until we run out of devices to check.
		//However, just in case some unexpected error occurs, keep track of the number of loops executed.
		//If the number of loops exceeds a very large number, exit anyway, to prevent inadvertent infinite looping.
		LoopCounter++;
		if (LoopCounter == 10000000)	//Surely there aren't more than 10 million devices attached to any forseeable PC...
		{
			return FALSE;
		}
	} // end of while(true)
}


int USBDEV::open_connection(int index) {
	//Now perform an initial start up check of the device state (attached or not attached), since we would not have
	//received a WM_DEVICECHANGE notification.
	if (CheckIfPresentAndGetUSBDevicePath((DWORD)index)) {	//Check and make sure at least one device with matching VID/PID is attached
		//We now have the proper device path, and we can finally open a device handle to the device.
		//WinUSB requires the device handle to be opened with the FILE_FLAG_OVERLAPPED attribute.
		MyDeviceHandle = CreateFile((DetailedInterfaceDataStructure->DevicePath), GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_FLAG_OVERLAPPED, 0);
		DWORD ErrorStatus = GetLastError();
		if (ErrorStatus == ERROR_SUCCESS) {
			//Now get the WinUSB interface handle by calling WinUsb_Initialize() and providing the device handle.
			BOOL BoolStatus = WinUsb_Initialize(MyDeviceHandle, &MyWinUSBInterfaceHandle);
			if (BoolStatus == TRUE) {
				ULONG timeout = 100; // ms
				WinUsb_SetPipePolicy(MyWinUSBInterfaceHandle, 0x81, PIPE_TRANSFER_TIMEOUT, sizeof(ULONG), &timeout);
				WinUsb_SetPipePolicy(MyWinUSBInterfaceHandle, 0x82, PIPE_TRANSFER_TIMEOUT, sizeof(ULONG), &timeout);
				if (ENABLE_FERSLIB_LOGMSG) {
					FERS_LibMsg((char*)"[ERROR] Device not found on USB\n");
				}			
			}		
		}
		IsOpen = TRUE;
	} else {	// Device must not be connected (or not programmed with correct firmware)				
		if (ENABLE_FERSLIB_LOGMSG) {
			FERS_LibMsg((char*)"[ERROR] Device not found on USB\n");
		}
		return FERSLIB_ERR_COMMUNICATION;
	}
	//if (streamMutex_initialized == 0) {
	//	initmutex(USBStream_Mutex);
	//	streamMutex_initialized = 1;
	//}
	return 0;
}


//-------------------------------------------------------------------
// Funzione di supporto per svuotare completamente l'endpoint 0x82
//-------------------------------------------------------------------
int USBDEV::drain_fifo(std::vector<char>& outData)
{
	// Read from endpoint 0x82 continously untill it results to be empty.
	// No other function can break the flow since we have already locked
	// the main_mutex
	unsigned char tempBuf[1024];
//	Sleep(50);
	while (true)
	{
		ULONG bytesTransferred = 0;
		BOOL status = WinUsb_ReadPipe(
			MyWinUSBInterfaceHandle,
			0x82,
			tempBuf,
			sizeof(tempBuf),
			&bytesTransferred,
			nullptr
		);

		// Se la lettura fallisce o non ha trasferito nulla, usciamo
		if (!status || bytesTransferred == 0)
		{
			break;
		}		
		
		// Append dei dati letti al vector di output
		outData.insert(outData.end(), tempBuf, tempBuf + bytesTransferred);
	}
	return 0; // o eventuale codice di errore
}


int USBDEV::set_service_reg(uint32_t address, uint32_t data) {
	BOOL Status;
	unsigned char OUTBuffer[512];		//BOOTLOADER HACK
	unsigned char INBuffer[512];		//BOOTLOADER HACK
	ULONG LengthTransferred;
	memcpy(&OUTBuffer[1], &address, 4);
	memcpy(&OUTBuffer[5], &data, 4);

	//Prepare a USB OUT data packet to send to the device firmware
	OUTBuffer[0] = 0x10;	//0x80 in byte 0 position is the "Toggle LED" command in the firmware

	//Now send the USB packet data in the OUTBuffer[] to the device firmware.
	Status = WinUsb_WritePipe(MyWinUSBInterfaceHandle, 0x01, &OUTBuffer[0], 1 + 4 + 4, &LengthTransferred, NULL); //BOOTLOADER HACK
	if (Status == FALSE) {
		if (ENABLE_FERSLIB_LOGMSG) {
			FERS_LibMsg((char*)"[ERROR] write pipe failed on USB\n");
		}
		return FERSLIB_ERR_COMMUNICATION;
	}

	Status = WinUsb_ReadPipe(MyWinUSBInterfaceHandle, 0x81, &INBuffer[0], 4, &LengthTransferred, NULL);
	/*if (Status == FALSE) {  // CTIN: the ReadPipe returns an error code, but the IP reset works fine. Can we ignore it?
		if (ENABLE_FERSLIB_LOGMSG) FERS_LibMsg("[ERROR] read pipe failed on USB\n");
		return FERSLIB_ERR_COMMUNICATION;
	}*/
	return 0;
}


int USBDEV::write_mem(uint32_t address, uint32_t length, char* data) {
	BOOL Status;
	unsigned char OUTBuffer[512];
	unsigned char INBuffer[512];
	ULONG LengthTransferred;
	memcpy(&OUTBuffer[1], &address, 4);
	memcpy(&OUTBuffer[5], &length, 4);
	memcpy(&OUTBuffer[9], data, length);

	//Prepare a USB OUT data packet to send to the device firmware
	OUTBuffer[0] = 0x80;	//0x80 in byte 0 position is the "Toggle LED" command in the firmware
	// Now send the USB packet data in the OUTBuffer[] to the device firmware.
	Status = WinUsb_WritePipe(MyWinUSBInterfaceHandle, 0x01, &OUTBuffer[0], 1 + 4 + 4 + length, &LengthTransferred, NULL);
	if (Status == FALSE) {
		if (ENABLE_FERSLIB_LOGMSG) {
			FERS_LibMsg((char*)"[ERROR] write pipe failed on USB\n");
		}
		return FERSLIB_ERR_COMMUNICATION;
	}
	//We successfully sent the request to the firmware, it is now time to
	//try to read the response IN packet from the device.
	Status = WinUsb_ReadPipe(MyWinUSBInterfaceHandle, 0x81, &INBuffer[0], 4, &LengthTransferred, NULL);
	if (Status == FALSE) {
		if (ENABLE_FERSLIB_LOGMSG) {
			FERS_LibMsg((char*)"[ERROR] read pipe failed on USB\n");
		}
		return FERSLIB_ERR_COMMUNICATION;
	}
	return 0;
}


int USBDEV::read_mem(uint32_t address, uint32_t length, char* data) {
	unsigned char OUTBuffer[512];
	unsigned char INBuffer[512];
	ULONG BytesTransferred;
	BOOL Status;
	//send a packet back IN to us, with the pushbutton state information
	OUTBuffer[0] = 0x81;	//0x81
	memcpy(&OUTBuffer[1], &address, 4);
	memcpy(&OUTBuffer[5], &length, 4);
	Status = WinUsb_WritePipe(MyWinUSBInterfaceHandle, 0x01, &OUTBuffer[0], 9, &BytesTransferred, NULL);
	// Do error case checking to verify that the packet was successfully sent
	if (Status == FALSE) {
		if (ENABLE_FERSLIB_LOGMSG) {
			FERS_LibMsg((char*)"[ERROR] write pipe failed on USB\n");
		}
		return FERSLIB_ERR_COMMUNICATION;
	}
	//We successfully sent the request to the firmware, it is now time to
	//try to read the response IN packet from the device.
	Status = WinUsb_ReadPipe(MyWinUSBInterfaceHandle, 0x81, &INBuffer[0], length, &BytesTransferred, NULL);
	if (Status == FALSE) {
		if (ENABLE_FERSLIB_LOGMSG) {
			FERS_LibMsg((char*)"[ERROR] read pipe failed on USB\n");
		}
		return FERSLIB_ERR_COMMUNICATION;
	}
	memcpy(data, INBuffer, length);
	return 0;
}


//-------------------------------------------------------------------
// write_reg: scrive un registro (4 byte) a 'address'
//-------------------------------------------------------------------
int USBDEV::write_reg(uint32_t address, uint32_t data)
{
	std::lock_guard<std::mutex> guard(main_mutex);

	//if (trylock(USBStream_Mutex) != 0);

	// Prima di scrivere un registro, occorre disabilitare lo stream,
	// svuotare la FIFO e poi scrivere. In questo esempio, se serve,
	// lo facciamo esattamente come in read_reg.

	bool wasStreaming = streaming_active;
	if (wasStreaming)
	{
		stream_enable2(false);

		// Svuota FIFO
		std::vector<char> tmpData;
		drain_fifo(tmpData);

		// Accoda il leftover al buffer interno, per non perdere dati
		leftover_data.insert(leftover_data.end(), tmpData.begin(), tmpData.end());
	}


	// Scrittura del registro (4 bytes)
	int result = write_mem(address, 4, reinterpret_cast<char*>(&data));

	// Riabilita lo stream se era attivo
	if (wasStreaming)
	{
		//stream_enable2(true);
		for (int i = 0; i < 10; i++) {
			stream_enable2(true);
		}
	}

	return result;
}


//-------------------------------------------------------------------
// read_reg: legge un registro (4 byte) da 'address'
//-------------------------------------------------------------------
int USBDEV::read_reg(uint32_t address, uint32_t* data)
{
	std::lock_guard<std::mutex> guard(main_mutex);

	// Se lo stream era abilitato, lo disabilitiamo
	bool wasStreaming = streaming_active;
	if (wasStreaming)
	{
		stream_enable2(false);

		// Svuota del tutto la FIFO su ep 0x82
		std::vector<char> tmpData;
		drain_fifo(tmpData);

		// Put the data into the leftover_data vector. In the next cycle, read_pipe 
		// (when the stream will be active again) will read the data, without loose them
		leftover_data.insert(leftover_data.end(), tmpData.begin(), tmpData.end());

	}

	// Ora possiamo leggere il registro senza che la FIFO "sporca" interferisca
	int result = read_mem(address, 4, reinterpret_cast<char*>(data));

	// Riabilita lo stream se era attivo
	if (wasStreaming)
	{
		for (int i = 0; i < 10; i++) {
			stream_enable2(true);
		}
		//stream_enable2(true);
	}

	return result;
}


//-------------------------------------------------------------------
// stream_enable: abilita o disabilita lo stream (endpoint 0x82)
//-------------------------------------------------------------------
int USBDEV::stream_enable2(bool enable)
{
	// RIMOSSO il lock, ci aspettiamo che la funzione
	// venga sempre chiamata da codice che ha gia preso main_mutex

	BOOL Status;
	unsigned char OUTBuffer[64];
	ULONG LengthTransferred;

	OUTBuffer[0] = 0xFA;
	OUTBuffer[1] = enable ? 0x01 : 0x00;

	Status = WinUsb_WritePipe(
		MyWinUSBInterfaceHandle,
		0x01,
		OUTBuffer,
		2,
		&LengthTransferred,
		NULL
	);
	if (!Status)
	{
		// gestisci errore
		return FERSLIB_ERR_COMMUNICATION;
	}

	streaming_active = enable;
	return 0;
}


int USBDEV::stream_enable(bool enable)
{
	// RIMOSSO il lock, ci aspettiamo che la funzione
	// venga sempre chiamata da codice che ha gia preso main_mutex
	std::lock_guard<std::mutex> guard(main_mutex);
	BOOL Status;
	unsigned char OUTBuffer[64];
	ULONG LengthTransferred;

	OUTBuffer[0] = 0xFA;
	OUTBuffer[1] = enable ? 0x01 : 0x00;

	Status = WinUsb_WritePipe(
		MyWinUSBInterfaceHandle,
		0x01,
		OUTBuffer,
		2,
		&LengthTransferred,
		NULL
	);
	if (!Status)
	{
		// gestisci errore
		return FERSLIB_ERR_COMMUNICATION;
	}

	streaming_active = enable;
	return 0;
}


//-------------------------------------------------------------------
// read_pipe: legge dallo stream (endpoint 0x82). Va in concorrenza
// con eventuali read_reg/write_reg, da cui ci proteggiamo con il mutex
//-------------------------------------------------------------------
int USBDEV::read_pipe(char* buff, int size, int* nb)
{
	//std::lock_guard<std::mutex> guard(main_mutex);
	std::unique_lock<std::mutex> guard(main_mutex, std::try_to_lock);
	if (!guard.owns_lock()) {
		*nb = 0;
		return 2; // Mutex gia bloccato, ritorna subito
	}

	if (nb) *nb = 0;
	if (size <= 0)
		return 0;

	int bytes_read = 0;

	// 1) Se leftover_data non e' vuoto, restituisco (tutto o parte) di quei dati
	if (!leftover_data.empty())
	{
		const int leftoverSize = static_cast<int>(leftover_data.size());
		const int toCopy = min(size, leftoverSize);

		// Copio toCopy byte da leftover_data -> buff
		std::memcpy(buff, leftover_data.data(), toCopy);
		bytes_read = toCopy;

		// Rimuovo i byte appena copiati dal leftover_data
		leftover_data.erase(leftover_data.begin(),
			leftover_data.begin() + toCopy);

		// Se abbiamo riempito tutto il buffer dellutente,
		// ci fermiamo qui e torniamo subito
		if (nb) *nb = bytes_read;
		return 0;
	}

	// 2) If leftover_data empty, read from the board
	if (streaming_active)
	{
		ULONG BytesTransferred = 0;
		BOOL status = WinUsb_ReadPipe(
			MyWinUSBInterfaceHandle,
			0x82,
			reinterpret_cast<unsigned char*>(buff),
			size,
			&BytesTransferred,
			nullptr
		);

		if (status && BytesTransferred > 0)
		{
			bytes_read = static_cast<int>(BytesTransferred);
		}
	}
	// If streaming is not active or status==FALSE, bytes_read staies at 0

	if (nb) *nb = bytes_read;
	return 0;
}


void USBDEV::close_connection() {
	if (MyWinUSBInterfaceHandle) {
		WinUsb_Free(MyWinUSBInterfaceHandle);
		MyWinUSBInterfaceHandle = nullptr;
	}
	if (MyDeviceHandle != INVALID_HANDLE_VALUE) {
		CloseHandle(MyDeviceHandle);
		MyDeviceHandle = INVALID_HANDLE_VALUE;
	}
	IsOpen = FALSE;
}

#else

libusb_context* USBDEV::ctx = nullptr;
unsigned int USBDEV::occurency = 0;


bool	USBDEV::CheckIfPresentAndGetUSBDevicePath(int InterfaceIndex, libusb_device_handle** dev_handle) {
	libusb_device** devs; //pointer to pointer of device, used to retrieve a list of devices
	int r; //for return values
	int i = 0, fersIdx = 0;
	ssize_t cnt; //holding number of devices in list
	if (ctx == nullptr) {
		r = libusb_init(&ctx); //initialize a library session
		if (r < 0) {
			return false;
		}
	}
	cnt = libusb_get_device_list(ctx, &devs); //get the list of devices
	if (cnt < 0) {
		libusb_free_device_list(devs, 1); //free the list, unref the devices in it
		libusb_exit(ctx); //close the session
		return false;
	}
	if (InterfaceIndex >= cnt) {
		libusb_free_device_list(devs, 1); //free the list, unref the devices in it
		libusb_exit(ctx); //close the session
		return false;
	}
	libusb_device_descriptor desc;
	while (i < cnt) {
		r = libusb_get_device_descriptor(devs[i], &desc);
		if (r < 0) {
			libusb_free_device_list(devs, 1); //free the list, unref the devices in it
			libusb_exit(ctx); //close the session
			return false;
		}
		if ((desc.idVendor == 0x04D8) && (desc.idProduct == 0x53)) {
			if (InterfaceIndex != fersIdx) {
				++fersIdx;
				++i;
				continue;
			}
			r = libusb_open((libusb_device*)(devs[i]), dev_handle);
			if (r != 0) {
				libusb_free_device_list(devs, 1); //free the list, unref the devices in it
				if (InterfaceIndex == 0) libusb_exit(ctx); //close the session
				return false;
			}
			if (libusb_kernel_driver_active(*dev_handle, 0) == 1) { //find out if kernel driver is attached
				if (libusb_detach_kernel_driver(*dev_handle, 0) != 0) {
					libusb_close(*dev_handle);
					libusb_free_device_list(devs, 1); //free the list, unref the devices in it
					if (InterfaceIndex == 0) {
						libusb_exit(ctx); //close the session
						ctx = nullptr;
					}
					return false;
				}
			}
			r = libusb_claim_interface(*dev_handle, 0);
			if (r < 0) {
				libusb_close(*dev_handle);
				libusb_free_device_list(devs, 1); //free the list, unref the devices in it
				if (InterfaceIndex == 0) {
					libusb_exit(ctx); //close the session
					ctx = nullptr;
				}
				return false;
			}
			if (InterfaceIndex == fersIdx) {
				occurency++;
				libusb_free_device_list(devs, 1); //free the list, unref the devices in it
				return true;
			}
		}
		i++;
	}
	libusb_free_device_list(devs, 1); //free the list, unref the devices in it
	if (occurency == 0) {
		libusb_exit(ctx); //close the session
		ctx = nullptr;
	}
	return false;
}


int USBDEV::USBSend(unsigned char* outBuffer, unsigned char* inBuffer, int outsize, int insize) {
	int r;
	int actual; //used to find out how many bytes were written

	r = libusb_bulk_transfer(dev_handle, 0x1, outBuffer, outsize, &actual, TIMEOUT);
	if (!(r == 0 && actual == outsize)) {
		if (ENABLE_FERSLIB_LOGMSG) {
			FERS_LibMsg((char*)"[ERROR] write pipe failed on USB\n");
		}
		return FERSLIB_ERR_COMMUNICATION;
	}
	r = libusb_bulk_transfer(dev_handle, 0x81, inBuffer, insize, &actual, TIMEOUT);
	if (!(r == 0 && actual == insize)) {
		if (ENABLE_FERSLIB_LOGMSG) {
			FERS_LibMsg((char*)"[ERROR] read pipe failed on USB\n");
		}
		return FERSLIB_ERR_COMMUNICATION;
	}
	return 0;

}


USBDEV::USBDEV() {
}

int USBDEV::open_connection(int index) {
	if (USBDEV::CheckIfPresentAndGetUSBDevicePath(index, &dev_handle)) {	//Check and make sure at least one device with matching VID/PID is attached
		IsOpen = true;
	} else {	// Device must not be connected (or not programmed with correct firmware)
		if (ENABLE_FERSLIB_LOGMSG) {
			FERS_LibMsg((char*)"[ERROR] Device not found on USB\n");
		}
		return FERSLIB_ERR_COMMUNICATION;
	}
	return 0;
}


void USBDEV::close_connection() {
	if (IsOpen) {
		occurency--;
		libusb_close(dev_handle);
	}
	if (occurency == 0)
		if (ctx != nullptr) libusb_exit(ctx); //close the session
	ctx = nullptr;
}


int USBDEV::drain_fifo(std::vector<char>& outData) {
	// Read continously the endpoint x untill it is empty.
	// Main mutex is already acquired outside, so no other function
	// can interfere while draining.
	unsigned char tempBuf[1024];
	int actualLength = 0;
	while (true) {
		int status = libusb_bulk_transfer(
			dev_handle,
			0x82,
			tempBuf,
			sizeof(tempBuf),
			&actualLength,
			TIMEOUT
		);
		// If the read fails or nothing is transferred, exit
		if (status != 0 || actualLength == 0) {
			break;
		}
		// Append the read data to the output vector
		outData.insert(outData.end(), tempBuf, tempBuf + actualLength);
	}
	return 0;
}


int USBDEV::set_service_reg(uint32_t address, uint32_t data) {
	unsigned char OUTBuffer[512];		//BOOTLOADER HACK
	unsigned char INBuffer[512];		//BOOTLOADER HACK
	int actual; //used to find out how many bytes were written

	memcpy(&OUTBuffer[1], &address, 4);
	memcpy(&OUTBuffer[5], &data, 4);

	//Prepare a USB OUT data packet to send to the device firmware
	OUTBuffer[0] = 0x10;	//0x80 in byte 0 position is the "Toggle LED" command in the firmware

	//Now send the USB packet data in the OUTBuffer[] to the device firmware.
	return USBDEV::USBSend(OUTBuffer, INBuffer, 1 + 4 + 4, 4);
}


int USBDEV::write_mem(uint32_t address, uint32_t length, char* data) {
	unsigned char OUTBuffer[512];
	unsigned char INBuffer[512];
	memcpy(&OUTBuffer[1], &address, 4);
	memcpy(&OUTBuffer[5], &length, 4);
	memcpy(&OUTBuffer[9], data, length);

	//Prepare a USB OUT data packet to send to the device firmware
	OUTBuffer[0] = 0x80;	//0x80 in byte 0 position is the "Toggle LED" command in the firmware
	// Now send the USB packet data in the OUTBuffer[] to the device firmware.
	return USBSend(OUTBuffer, INBuffer, 1 + 4 + 4 + length, 4);
}


int USBDEV::read_mem(uint32_t address, uint32_t length, char* data) {
	volatile unsigned char PushbuttonStateByte = 0xFF;
	unsigned char OUTBuffer[512];
	unsigned char INBuffer[512];

	//send a packet back IN to us, with the pushbutton state information
	OUTBuffer[0] = 0x81;	//0x81
	memcpy(&OUTBuffer[1], &address, 4);
	memcpy(&OUTBuffer[5], &length, 4);

	if (USBDEV::USBSend(OUTBuffer, INBuffer, 1 + 4 + 4, length) == 0) {
		memcpy(data, INBuffer, length);
		return 0;
	}
	return FERSLIB_ERR_COMMUNICATION;
}


int USBDEV::write_reg(uint32_t address, uint32_t data) {
	// Before write a register:
	// - disable data stream
	// - drain the FIFO
	// - write the register
	// - re-enable the data stream
	// Same approach for read_reg
	std::lock_guard<std::mutex> guard(main_mutex);

	// Disable stream if it is active
	bool wasStreaming = streaming_active;
	if (wasStreaming) {
		stream_enable2(false);

		// Drain FIFO
		std::vector<char> tmpData;
		drain_fifo(tmpData);
		// Append the leftover to the internal buffer, to not lose data
		leftover_data.insert(leftover_data.end(), tmpData.begin(), tmpData.end());
	}
	// Do the register write
	int result = write_mem(address, 4, (char*)&data);

	// Re-enable the data stream if it was active
	if (wasStreaming) {
		for (int i = 0; i < 10; i++) {
			stream_enable2(true);
		}
	}

	return result;
}



int USBDEV::read_reg(uint32_t address, uint32_t* data) {
	// Before read a register:
	// - disable data stream
	// - drain the FIFO
	// - read the register
	// - re-enable the data stream
	std::lock_guard<std::mutex> guard(main_mutex);

	// Disable stream if it is active
	bool wasStreaming = streaming_active;
	if (wasStreaming) {
		stream_enable2(false);
		// Drain FIFO
		std::vector<char> tmpData;
		drain_fifo(tmpData);
		// Append the leftover to the internal buffer, to not lose data
		leftover_data.insert(leftover_data.end(), tmpData.begin(), tmpData.end());
	}
	// Do the register read
	int result = read_mem(address, 4, (char*)data);

	// Re-enable the data stream if it was active
	if (wasStreaming) {
		for (int i = 0; i < 10; i++) {
			stream_enable2(true);
		}
	}
	return result;
}


int USBDEV::stream_enable2(bool enable) {
	// RIMOSSO il lock, ci aspettiamo che la funzione
	// venga sempre chiamata da codice che ha gia preso main_mutex
	unsigned char OUTBuffer[64];
	int r;
	int actual; //used to find out how many bytes were written

	//Prepare a USB OUT data packet to send to the device firmware
	OUTBuffer[0] = 0xFA;	//0x79 Stream control
	OUTBuffer[1] = enable ? 0x01 : 0x00;	//enable

	r = libusb_bulk_transfer(dev_handle, 0x1, OUTBuffer, 2, &actual, TIMEOUT);
	if (!(r == 0 && actual == 2)) {
		if (ENABLE_FERSLIB_LOGMSG) {
			FERS_LibMsg((char*)"[ERROR] write pipe failed on USB\n");
		}
		return FERSLIB_ERR_COMMUNICATION;
	}
	streaming_active = enable;
	return 0;
}


int USBDEV::stream_enable(bool enable) {
	std::lock_guard<std::mutex> guard(main_mutex);

	unsigned char OUTBuffer[64];
	int r;
	int actual; //used to find out how many bytes were written

	//Prepare a USB OUT data packet to send to the device firmware
	OUTBuffer[0] = 0xFA;	//0x79 Stream control
	OUTBuffer[1] = enable ? 0x01 : 0x00;	//enable

	r = libusb_bulk_transfer(dev_handle, 0x1, OUTBuffer, 2, &actual, TIMEOUT);
	if (!(r == 0 && actual == 2)) {
		if (ENABLE_FERSLIB_LOGMSG) {
			FERS_LibMsg((char*)"[ERROR] write pipe failed on USB\n");
		}
		return FERSLIB_ERR_COMMUNICATION;
	}
	streaming_active = enable;
	return 0;
}


//-------------------------------------------------------------------
// read_pipe: legge dallo stream (endpoint 0x82). Va in concorrenza
// con eventuali read_reg/write_reg, da cui ci proteggiamo con il mutex
//-------------------------------------------------------------------
int USBDEV::read_pipe(char* buff, int size, int* nb) {
	std::unique_lock<std::mutex> guard(main_mutex, std::try_to_lock);
	if (!guard.owns_lock()) {
		*nb = 0;
		return 2; // Mutex already locked, return 2
	}

	int r;
	int actual;

	*nb = 0;
	if (size == 0) return 0;

	int bytes_read = 0;
	// 1) Se leftover_data non vuoto, restituisco (tutto o parte) di quei dati
	if (!leftover_data.empty()) {
		const int leftoverSize = static_cast<int>(leftover_data.size());
		const int toCopy = min(size, leftoverSize);

		// Copio toCopy byte da leftover_data -> buff
		std::memcpy(buff, leftover_data.data(), toCopy);
		bytes_read = toCopy;

		// Rimuovo i byte appena copiati dal leftover_data
		leftover_data.erase(leftover_data.begin(), leftover_data.begin() + toCopy);

		// Se abbiamo riempito tutto il buffer dellutente,
		// ci fermiamo qui e torniamo subito
		if (nb) *nb = bytes_read;
		return 0;
	}

	// 2) IF leftover_Data is empty, read data from the board
	if (streaming_active) {
		//Stream read on ep 0x82
		r = libusb_bulk_transfer(dev_handle, 0x82, (unsigned char*)buff, size, &actual, TIMEOUT);
		if ((r != 0) && (r != -7)) {
			if (ENABLE_FERSLIB_LOGMSG) {
				FERS_LibMsg((char*)"[ERROR] read pipe failed on USB\n");
			}
			return FERSLIB_ERR_COMMUNICATION;
		}
	}
	*nb = actual;
	return 0;
}

#endif

// *********************************************************************************************************
// END OF C++ SECTION
// *********************************************************************************************************

// *********************************************************************************************************
// Global variables
// *********************************************************************************************************
static USBDEV FERS_usb[FERSLIB_MAX_NBRD];
static char* RxBuff[FERSLIB_MAX_NBRD][2] = { NULL };	// Rx data buffers (two "ping-pong" buffers, one write, one read)
static uint32_t RxBuff_rp[FERSLIB_MAX_NBRD] = { 0 };	// read pointer in Rx data buffer
static uint32_t RxBuff_wp[FERSLIB_MAX_NBRD] = { 0 };	// write pointer in Rx data buffer
static int RxB_w[FERSLIB_MAX_NBRD] = { 0 };				// 0 or 1 (which is the buffer being written)
static int RxB_r[FERSLIB_MAX_NBRD] = { 0 };				// 0 or 1 (which is the buffer being read)
static int RxB_Nbytes[FERSLIB_MAX_NBRD][2] = { 0 };		// Number of bytes written in the buffer
static int WaitingForData[FERSLIB_MAX_NBRD] = { 0 };	// data consumer is waiting fro data (data receiver should flush the current buffer)
static int RxStatus[FERSLIB_MAX_NBRD] = { 0 };			// 0=not started, 1=idle (wait for run), 2=running (taking data), 3=closing run (reading data in the pipes)
static int QuitThread[FERSLIB_MAX_NBRD] = { 0 };		// Quit Thread
static f_sem_t RxSemaphore[FERSLIB_MAX_NBRD];			// Semaphore for sync the data receiver thread
//static f_sem_t StartSemaphore[FERSLIB_MAX_NBRD];		// Semaphore for sync the data receiver thread with the start sent
static f_thread_t ThreadID[FERSLIB_MAX_NBRD];			// RX Thread ID
static mutex_t RxMutex[FERSLIB_MAX_NBRD];				// Mutex for the access to the Rx data buffer and pointers
static FILE* Dump[FERSLIB_MAX_NBRD] = { NULL };			// low level data dump files (for debug)
static FILE* RawData[FERSLIB_MAX_NBRD] = { NULL };		// Raw Data output file, used for further reprocessing
static uint8_t ReadData_Init[FERSLIB_MAX_NBRD] = { 0 }; // Re-init read pointers after run stop
static int subrun[FERSLIB_MAX_NBRD] = { 0 };			// Sub Run index
static int64_t size_file[FERSLIB_MAX_NBRD] = { 0 };		// Size of Raw data output file
static std::mutex rdf_mutex[FERSLIB_MAX_NBRD];			// Mutex to access RawData file

#define USB_BLK_SIZE  (512 * 8)							// Max size of one USB packet (4k)
#define RX_BUFF_SIZE  (16 * USB_BLK_SIZE)				// Size of the local Rx buffer

// ********************************************************************************************************
// Write messages or data to debug dump file
// ********************************************************************************************************
static int FERS_DebugDump(int bindex, const char* fmt, ...) {
	char msg[1000], filename[200];
	static int openfile[FERSLIB_MAX_NBRD] = { 1 };
	va_list args;
	if (bindex >= FERSLIB_MAX_NBRD) return -1;
	if (openfile[bindex]) {
		sprintf(filename, "ll_log_%d.txt", bindex);
		Dump[bindex] = fopen(filename, "w");
		openfile[bindex] = 0;
	}

	va_start(args, fmt);
	vsprintf(msg, fmt, args);
	va_end(args);
	if (Dump[bindex] != NULL)
		fprintf(Dump[bindex], "%s", msg);
	return 0;
}


// ********************************************************************************************************
// Utility for increase subrun number for RawData output file
// ********************************************************************************************************
// --------------------------------------------------------------------------------------------------------
// Description: Close and Open a new RawData output file increasing the subrun number
// Inputs:		None
// Return:		0
// --------------------------------------------------------------------------------------------------------
int LLusb_IncreaseRawDataSubrun(int bidx) {
	fclose(RawData[bidx]);
	++subrun[bidx];
	size_file[bidx] = 0;
	char filename[200];
	sprintf(filename, "%s.%d.frd", RawDataFilename[bidx], subrun[bidx]);
	RawData[bidx] = fopen(filename, "wb");
	return 0;
}

// *********************************************************************************************************
// R/W Memory and Registers
// *********************************************************************************************************
// --------------------------------------------------------------------------------------------------------- 
// Description: Write a memory block to the FERS board
// Inputs:		bindex = FERS index
//				address = mem address (1st location)
//				data = reg data
//				size = num of bytes being written
// Return:		0=OK, negative number = error code
// --------------------------------------------------------------------------------------------------------- 
int LLusb_WriteMem(int bindex, uint32_t address, char* data, uint16_t size) {
	return FERS_usb[usbIdx[bindex]].write_mem(address, size, data);
}

// --------------------------------------------------------------------------------------------------------- 
// Description: Read a memory block from the FERS board 
// Inputs:		bindex = FERS index
//				address = mem address (1st location)
//				size = num of bytes being written
// Outputs:		data = reg data
// Return:		0=OK, negative number = error code
// --------------------------------------------------------------------------------------------------------- 
int LLusb_ReadMem(int bindex, uint32_t address, char* data, uint16_t size) {
	return FERS_usb[usbIdx[bindex]].read_mem(address, size, data);
}

// --------------------------------------------------------------------------------------------------------- 
// Description: Write a register of the FERS board 
// Inputs:		bindex = FERS index
//				address = reg address 
//				data = reg data
// Return:		0=OK, negative number = error code
// --------------------------------------------------------------------------------------------------------- 
int LLusb_WriteRegister(int bindex, uint32_t address, uint32_t data) {
	return FERS_usb[usbIdx[bindex]].write_reg(address, data);
}


// --------------------------------------------------------------------------------------------------------- 
// Description: Read a register of the FERS board 
// Inputs:		bindex = FERS index
//				address = reg address 
// Outputs:		data = reg data
// Return:		0=OK, negative number = error code
// --------------------------------------------------------------------------------------------------------- 
int LLusb_ReadRegister(int bindex, uint32_t address, uint32_t* data) {
	return FERS_usb[usbIdx[bindex]].read_reg(address, data);
}


// *********************************************************************************************************
// Raw data readout
// *********************************************************************************************************
// Thread that keeps reading data from the data socket (at least until the Rx buffer gets full)
static void* usb_data_receiver(void* params) {
	int bindex = *(int*)params;
	int nbreq, nbrx, nbfree, stream = 0, ret, empty = 0;
	int FlushBuffer = 0;
	int WrReady = 1;

	int start_sent = 0;
	char* wpnt;
	uint64_t ct, pt, tstart = 0, tstop = 0, tdata = 0;

	if ((DebugLogs & DBLOG_LL_DATADUMP) || (DebugLogs & DBLOG_LL_MSGDUMP)) {
		char filename[200];
		sprintf(filename, "ll_log_%d.txt", bindex);
		Dump[bindex] = fopen(filename, "w");
	}

	if (ENABLE_FERSLIB_LOGMSG) FERS_LibMsg((char*)"[INFO][BRD %02d] Data receiver thread is started\n", bindex);
	if (DebugLogs & DBLOG_LL_MSGDUMP) FERS_DebugDump(bindex, "RX thread started\n");
	f_sem_post(&RxSemaphore[bindex]);

	lock(RxMutex[bindex]);
	RxStatus[bindex] = RXSTATUS_IDLE;
	unlock(RxMutex[bindex]);

	f_sem_init(&FERS_StartRunSemaphore[bindex]);	// Init semaphore for sync the data receiver thread with the start sent

	ct = get_time();
	pt = ct;
	while (1) {
		ct = get_time();
		lock(RxMutex[bindex]);
		if (QuitThread[bindex]) {
			if (ENABLE_FERSLIB_LOGMSG) FERS_LibMsg((char*)"[INFO][BRD %02d] Data receiver thread is quitting\n", bindex);
			break;
		}
		if (RxStatus[bindex] == RXSTATUS_IDLE) {
			//if (stream == 0) {
			//	FERS_usb[bindex].stream_enable(true);
			//	stream = 1;
			//}
			if ((FERS_ReadoutStatus == ROSTATUS_RUNNING) && empty) {  // start of run
				if (stream == 0) {
					FERS_usb[usbIdx[bindex]].stream_enable(true);
					stream = 1;
				}
				// Clear Buffers
				ReadData_Init[bindex] = 1;
				RxBuff_rp[bindex] = 0;
				RxBuff_wp[bindex] = 0;
				RxB_r[bindex] = 0;
				RxB_w[bindex] = 0;
				WaitingForData[bindex] = 0;
				WrReady = 1;
				FlushBuffer = 0;
				for (int i = 0; i < 2; i++)
					RxB_Nbytes[bindex][i] = 0;
				tstart = ct;
				tdata = ct;
				if (DebugLogs & DBLOG_LL_MSGDUMP) {
					char st[100];
					time_t ss;
					time(&ss);
					strcpy(st, asctime(gmtime(&ss)));
					st[strlen(st) - 1] = 0;
					FERS_DebugDump(bindex, "\nRUN_STARTED at %s\n", st);
				}
				RxStatus[bindex] = RXSTATUS_RUNNING;
				lock(FERS_RoMutex);
				FERS_RunningCnt++;
				unlock(FERS_RoMutex);
			} else {
				unlock(RxMutex[bindex]);
				// make "dummy" reads while not running to prevent the USB FIFO to get full and become insensitive to any reg access (regs and data pass through the same pipe)
				if (!empty) {
					start_sent = 0;
					if (stream == 0) {
						FERS_usb[usbIdx[bindex]].stream_enable(true);
						stream = 1;
					}
					FERS_usb[usbIdx[bindex]].reset_leftover_data();
					ret = FERS_usb[usbIdx[bindex]].read_pipe(RxBuff[bindex][0], USB_BLK_SIZE, &nbrx);
					if (nbrx == 0 && ret == 0) empty = 1;
					if (!empty && (DebugLogs & DBLOG_LL_MSGDUMP)) FERS_DebugDump(bindex, "Reading old data...\n");
					if (stream == 1) {
						FERS_usb[usbIdx[bindex]].stream_enable(false);
						stream = 0;
					}
				}
				Sleep(10);
				continue;
			}
		}

		if ((RxStatus[bindex] == RXSTATUS_RUNNING) && (FERS_ReadoutStatus != ROSTATUS_RUNNING)) {  // stop of run 
			tstop = ct;
			RxStatus[bindex] = RXSTATUS_EMPTYING;
			if (DebugLogs & DBLOG_LL_MSGDUMP) FERS_DebugDump(bindex, "Board Stopped. Emptying data (T=%.3f)\n", 0.001 * (tstop - tstart));
		}

		if (RxStatus[bindex] == RXSTATUS_EMPTYING) {
			// stop RX for one of these conditions:
			//  - flush command 
			//  - there is no data for more than NODATA_TIMEOUT
			//  - STOP_TIMEOUT after the stop to the boards
			if ((FERS_ReadoutStatus == ROSTATUS_FLUSHING) || ((ct - tdata) > NODATA_TIMEOUT) || ((ct - tstop) > STOP_TIMEOUT)) {
				RxStatus[bindex] = RXSTATUS_IDLE;
				lock(FERS_RoMutex);
				if (FERS_RunningCnt > 0) FERS_RunningCnt--;
				unlock(FERS_RoMutex);
				if (DebugLogs & DBLOG_LL_MSGDUMP) FERS_DebugDump(bindex, "Run stopped (T=%.3f)\n", 0.001 * (ct - tstart));
				empty = 0;
				unlock(RxMutex[bindex]);
				continue;
			}
		}
		if (!WrReady) {  // end of current buff reached => switch to the other buffer (if available for writing)
			if (RxB_Nbytes[bindex][RxB_w[bindex]] > 0) {  // the buffer is not empty (being used for reading) => retry later
				unlock(RxMutex[bindex]);
				Sleep(10);
				continue;
			}
			WrReady = 1;
			//if (!stream) {
			//	FERS_usb[bindex].stream_enable(true);
			//	stream = 1;
			//}
		}

		if (!start_sent) {
			while (!start_sent) {
				f_sem_wait(&FERS_StartRunSemaphore[bindex], INFINITE);  // wait for the start signal
				start_sent = 1;
			}
		}

		wpnt = RxBuff[bindex][RxB_w[bindex]] + RxBuff_wp[bindex];
		nbfree = RX_BUFF_SIZE - RxBuff_wp[bindex];  // bytes free in the buffer
		nbreq = min(nbfree, USB_BLK_SIZE);

		unlock(RxMutex[bindex]);
		ret = FERS_usb[usbIdx[bindex]].read_pipe(wpnt, nbreq, &nbrx);
		if (ret != 0 && ret != 2) {
			lock(RxMutex[bindex]);
			RxStatus[bindex] = -1;
			if (ENABLE_FERSLIB_LOGMSG) FERS_LibMsg((char*)"[ERROR][BRD %02d] usb read pipe failed in data receiver thread\n", bindex);
			break;
		}

		if (0) {
			uint64_t pptime = 0;
			if ((ct - pptime) > 500) {
				printf(".");
				pptime = ct;
			}
		}

		if (nbrx > 0) tdata = ct;
		if ((nbfree < 4 * USB_BLK_SIZE) && (stream == 1)) FlushBuffer = 1;  // switch buffer if it has no space for at least 4 blocks

		lock(RxMutex[bindex]);
		RxBuff_wp[bindex] += nbrx;
		if ((ct - pt) > 10) {  // every 10 ms, check if the data consumer is waiting for data or if the thread has to quit
			if (QuitThread[bindex]) break;
			if (WaitingForData[bindex] && (RxBuff_wp[bindex] > 0)) FlushBuffer = 1;
			pt = ct;
		}

		if ((RxBuff_wp[bindex] == RX_BUFF_SIZE) || FlushBuffer) {  // the current buffer is full or must be flushed
			RxB_Nbytes[bindex][RxB_w[bindex]] = RxBuff_wp[bindex];
			RxB_w[bindex] ^= 1;  // switch to the other buffer
			RxBuff_wp[bindex] = 0;
			//FERS_usb[bindex].stream_enable(false);
			//stream = 0;
			WrReady = 0;
			FlushBuffer = 0;
		}
		// Dump data to log file (for debugging)
		if ((DebugLogs & DBLOG_LL_DATADUMP) && (nbrx > 0)) {
			for (int i = 0; i < nbrx; i += 4) {
				uint32_t* d32 = (uint32_t*)(wpnt + i);
				FERS_DebugDump(bindex, "%08X\n", *d32);
			}
			if (Dump[bindex] != NULL) fflush(Dump[bindex]);
		}
		// Saving Raw data output file
		if (FERScfg[bindex]->OF_RawData && !FERS_Offline) {
			size_file[bindex] += nbrx;
			if (FERScfg[bindex]->OF_LimitedSize > 0 && size_file[bindex] > FERScfg[bindex]->MaxSizeDataOutputFile) {
				std::lock_guard<std::mutex> guard(rdf_mutex[bindex]);
				LLusb_IncreaseRawDataSubrun(bindex);
				size_file[bindex] = nbrx;
			}
			std::lock_guard<std::mutex> guard(rdf_mutex[bindex]);
			if (RawData[bindex] != NULL) {
				fwrite(wpnt, sizeof(char), nbrx, RawData[bindex]);
				fflush(RawData[bindex]);
			}
		}

		unlock(RxMutex[bindex]);
	}
	unlock(RxMutex[bindex]);
	f_sem_destroy(&FERS_StartRunSemaphore[bindex]);
	if (Dump[bindex] != NULL) fclose(Dump[bindex]);
	return NULL;
}


// --------------------------------------------------------------------------------------------------------- 
// Description: Copy a data block from RxBuff of the FERS board to the user buffer 
// Inputs:		bindex = board index
//				buff = user data buffer to fill
//				maxsize = max num of bytes being transferred
//				nb = num of bytes actually transferred
// Return:		0=No Data, 1=Good Data 2=Not Running, negative number = error code
// --------------------------------------------------------------------------------------------------------- 
int LLusb_ReadData(int bindex, char* buff, int maxsize, int* nb) {
	char* rpnt;
	static int RdReady[FERSLIB_MAX_NBRD] = { 0 };
	static uint32_t Nbr[FERSLIB_MAX_NBRD] = { 0 };
	//static FILE *dd = NULL;

	*nb = 0;
	int a = trylock(RxMutex[bindex]);
	if (a != 0) return 0;
	if (ReadData_Init[bindex]) {
		RdReady[bindex] = 0;
		Nbr[bindex] = 0;
		ReadData_Init[bindex] = 0;
		unlock(RxMutex[bindex]);
		return 2;
	}

	if (RxStatus[bindex] != RXSTATUS_RUNNING) {
		unlock(RxMutex[bindex]);
		return 2;
	}
	if (!RdReady[bindex]) {
		if (RxB_Nbytes[bindex][RxB_r[bindex]] == 0) {  // The buffer is empty => assert "WaitingForData" and return 0 bytes to the caller
			WaitingForData[bindex] = 1;
			//FERS_usb[usbIdx[bindex]].stream_enable(true);
			unlock(RxMutex[bindex]);
			return 0;
		}
		RdReady[bindex] = 1;
		Nbr[bindex] = RxB_Nbytes[bindex][RxB_r[bindex]];  // Get the num of bytes available for reading in the buffer
		WaitingForData[bindex] = 0;
	}

	rpnt = RxBuff[bindex][RxB_r[bindex]] + RxBuff_rp[bindex];
	*nb = Nbr[bindex] - RxBuff_rp[bindex];  // num of bytes currently available for reading in RxBuff
	if (*nb > maxsize) *nb = maxsize;
	if (*nb > 0) {
		memcpy(buff, rpnt, *nb);
		RxBuff_rp[bindex] += *nb;
	}
	if (RxBuff_rp[bindex] == Nbr[bindex]) {  // end of current buff reached => switch to other buffer 
		RxB_Nbytes[bindex][RxB_r[bindex]] = 0;
		RxB_r[bindex] ^= 1;
		RxBuff_rp[bindex] = 0;
		RdReady[bindex] = 0;
	}
	unlock(RxMutex[bindex]);
	return 1;
}


int LLusb_ReadData_File(int bindex, char* buff, int maxsize, int* nb, int flushing) {
	//uint8_t stop_loop = 1;
	static int tmp_srun[FERSLIB_MAX_NBRD] = { 0 };
	static int fsizeraw[FERSLIB_MAX_NBRD] = { 0 };
	static FILE* ReadRawData[FERSLIB_MAX_NBRD] = { NULL };
	int fret = 0;
	if (flushing) {
		if (ReadRawData[bindex] != NULL)
			fclose(ReadRawData[bindex]);
		tmp_srun[bindex] = 0;
		return 0;
	}
	if (ReadRawData[bindex] == NULL) {	// Open Raw data file
		char filename[200];
		if (EnableSubRun)	// The Raw Data is the one FERS-like with subrun
			sprintf(filename, "%s.%d.frd", RawDataFilename[bindex], tmp_srun[bindex]);
		else	// The Raw Data has a custom name and there are not subrun
			sprintf(filename, "%s", RawDataFilename[bindex]);

		ReadRawData[bindex] = fopen(filename, "rb");
		if (ReadRawData[bindex] == NULL) {	// If the file is still NULL, no more subruns are available
			if (ENABLE_FERSLIB_LOGMSG) FERS_LibMsg((char*)"[INFO][BRD %02d] RawData reprocessing completed.\n", bindex);
			tmp_srun[bindex] = 0;
			return 4;
		}
		fseek(ReadRawData[bindex], 0, SEEK_END);	// Get the file size
		fsizeraw[bindex] = ftell(ReadRawData[bindex]);
		fseek(ReadRawData[bindex], 0, SEEK_SET);

		// Here the file has the correct format. For both files with FERSlib name format
		// and custom one, the header have to be search when tmp_srun is 0
		if (tmp_srun[bindex] == 0) {
			// Read Header keyword
			char file_header[50];
			fret = fread(&file_header, 32, 1, ReadRawData[bindex]);
			if (strcmp(file_header, "$$$$$$$FERSRAWDATAHEADER$$$$$$$") != 0) { // No header mark found
				if (ENABLE_FERSLIB_LOGMSG) FERS_LibMsg((char*)"[ERROR][BRD %02d] No valid header found in Raw Data filename %s\n", bindex, filename);
				_setLastLocalError("ERROR: No valid keyword header found");
				fclose(ReadRawData[bindex]);
				return FERSLIB_ERR_GENERIC;
			}
			size_t jump_size_header = 0;
			fret = fread(&jump_size_header, sizeof(jump_size_header), 1, ReadRawData[bindex]);
			fseek(ReadRawData[bindex], (long)jump_size_header, SEEK_CUR);

			fsizeraw[bindex] -= ftell(ReadRawData[bindex]);
		}
	}

	fsizeraw[bindex] -= maxsize;
	if (fsizeraw[bindex] < 0)	// Read what is missing from the current file
		maxsize = maxsize + fsizeraw[bindex];

	fret = fread(buff, sizeof(char), maxsize, ReadRawData[bindex]);

	if (fsizeraw[bindex] <= 0) {
		fclose(ReadRawData[bindex]);
		ReadRawData[bindex] = NULL;
		if (EnableSubRun)
			++tmp_srun[bindex];
		else {
			if (ENABLE_FERSLIB_LOGMSG) FERS_LibMsg((char*)"[INFO][BRD %02d] RawData reprocessing completed.\n");
			tmp_srun[bindex] = 0;
			return 4;
		}
	}
	*nb = maxsize;

	return 1;
}


// *********************************************************************************************************
// Open/Close
// *********************************************************************************************************
// --------------------------------------------------------------------------------------------------------- 
// Description: Open the Raw (LL) data output file
// Inputs:		bindex: board index
// Return:		0=OK, negative number = error code
// --------------------------------------------------------------------------------------------------------- 
int LLusb_OpenRawOutputFile(int handle) {
	if (ProcessRawData) return 0;

	int bidx = FERS_INDEX(handle);
	if (FERScfg[bidx]->OF_RawData && RawData[bidx] == NULL) {
		char filename[200];
		FERS_BoardInfo_t tmpInfo;
		FERS_GetBoardInfo(handle, &tmpInfo);
		sprintf(filename, "%s.%d.frd", RawDataFilename[bidx], subrun[bidx]);
		RawData[bidx] = fopen(filename, "wb");
		size_t header_size = sizeof(size_t) + sizeof(handle) + sizeof(tmpInfo);
		if (FERS_IsXROC(handle)) header_size += sizeof(PedestalLG[bidx]) + sizeof(PedestalHG[bidx]);

		char title[32] = "$$$$$$$FERSRAWDATAHEADER$$$$$$$";
		fwrite(&title, sizeof(title), 1, RawData[bidx]);
		fwrite(&header_size, sizeof(header_size), 1, RawData[bidx]);
		fwrite(&handle, sizeof(handle), 1, RawData[bidx]);
		fwrite(&tmpInfo, sizeof(tmpInfo), 1, RawData[bidx]);

		if (FERS_IsXROC(handle)) {
			fwrite(&PedestalLG[bidx], sizeof(PedestalLG[bidx]), 1, RawData[bidx]);
			fwrite(&PedestalHG[bidx], sizeof(PedestalHG[bidx]), 1, RawData[bidx]);
		}
	}
	return 0;
}


// --------------------------------------------------------------------------------------------------------- 
// Description: Close the Raw (LL) data output file
// Inputs:		bindex: board index
// Return:		0=OK, negative number = error code
// --------------------------------------------------------------------------------------------------------- 
int LLusb_CloseRawOutputFile(int handle) {
	if (ProcessRawData) return 0;

	int bidx = FERS_INDEX(handle);
	std::lock_guard<std::mutex> guard(rdf_mutex[bidx]);
	if (RawData[bidx] != NULL) {
		fclose(RawData[bidx]);
		RawData[bidx] = NULL;
	}
	size_file[bidx] = 0;
	subrun[bidx] = 0;
	return 0;
}


// --------------------------------------------------------------------------------------------------------- 
// Description: Open the direct connection to the FERS board through the USB interface. 
//				After the connection the function allocates the memory buffers starts the thread  
//				that receives the data
// Inputs:		PID = board PID
//				bindex = board index
// Return:		0=OK, negative number = error code
// --------------------------------------------------------------------------------------------------------- 
int LLusb_OpenDevice(int PID, int bindex) {
	int ret, started, i;
	f_thread_t threadID;
	static int OpenAllDevices = 1, Ndev = 0;

	USBDEV FERS_usb_temp;

	if (PID >= 10000) { // search for the board with the given PID between all the connected boards
		uint32_t d32;
		// 1st call => open all USB devices
		if (OpenAllDevices) {
			for (i = 0; i < FERSLIB_MAX_NBRD; i++) { // If you use FERScfg[brd]->NumBrd ?
				ret = FERS_usb[i].open_connection(i);
				if (ret != 0) break;
			}
			Ndev = i;
			OpenAllDevices = 0;
		}

		// DNIN: here at least!
		for (i = 0; i < Ndev; i++) {
			if (!FERS_usb[i].IsOpen)
				return FERSLIB_ERR_COMMUNICATION;  // no further connected board is found
			ret = FERS_usb[i].read_reg(a_pid, &d32);
			if (ret != 0) {
				FERS_LibMsg((char*)"[ERROR][BRD %02d] read PID failed\n", i);
				return FERSLIB_ERR_COMMUNICATION;  // no further connected board is found
			}
			if (d32 == (uint32_t)PID) break;
		}
		if (i == Ndev) {
			FERS_LibMsg((char*)"[ERROR][BRD %02d] No board found with PID %d\n", i, PID);
			return FERSLIB_ERR_COMMUNICATION;  // no board found with the given PID
		}
		// Swap indexes (i = board with wanted PID, bindex = wanted board index) 
		usbIdx[bindex] = i;
		//memcpy(&FERS_usb_temp, &FERS_usb[bindex], sizeof(USBDEV));
		//memcpy(&FERS_usb[bindex], &FERS_usb[i], sizeof(USBDEV));
		//memcpy(&FERS_usb[i], &FERS_usb_temp, sizeof(USBDEV));

		//if (!FERS_usb[bindex].IsOpen)
		//	return FERSLIB_ERR_COMMUNICATION;

	} else {  // open using consecutive index instead of PID
		ret = FERS_usb[bindex].open_connection(PID);
		if (ret != 0) {
			FERS_LibMsg((char*)"[ERROR][BRD %02d] Device not found on USB\n", bindex);
			return FERSLIB_ERR_COMMUNICATION;
		}
	}

	for (int j = 0; j < 2; j++) {
		RxBuff[bindex][j] = (char*)malloc(RX_BUFF_SIZE);
		FERS_TotalAllocatedMem += RX_BUFF_SIZE;
	}
	initmutex(RxMutex[bindex]);
	f_sem_init(&RxSemaphore[bindex]);

	QuitThread[bindex] = 0;
	//FERS_usb[bindex].stream_enable(true);
	thread_create(usb_data_receiver, &bindex, &threadID);
	started = 0;
	while (!started) {
		f_sem_wait(&RxSemaphore[bindex], INFINITE);
		started = 1;
	}
	//ret = LLusb_WriteRegister(bindex, a_commands, CMD_ACQ_STOP);	// stop acquisition (in case it was still running)
	//ret |= LLusb_WriteRegister(bindex, a_commands, CMD_CLEAR);		// clear data in the FPGA FIFOs

	f_sem_destroy(&RxSemaphore[bindex]);

	return 0;
}


// --------------------------------------------------------------------------------------------------------- 
// Description: Close the connection to the FERS board and free buffers
// Inputs:		bindex: board index
// Return:		0=OK, negative number = error code
// --------------------------------------------------------------------------------------------------------- 
int LLusb_CloseDevice(int bindex) {
	lock(RxMutex[bindex]);
	QuitThread[bindex] = 1;
	unlock(RxMutex[bindex]);
	for (int i = 0; i < 100; i++) {
		if (RxStatus[bindex] == RXSTATUS_OFF) break;
		Sleep(1);
	}

	FERS_usb[usbIdx[bindex]].stream_enable(false);
	if (&RxMutex[bindex] != NULL) destroymutex(RxMutex[bindex]);
	FERS_usb[usbIdx[bindex]].close_connection();
	for (int i = 0; i < 2; i++) {
		if (RxBuff[bindex][i] != NULL) {
			free(RxBuff[bindex][i]);
			RxBuff[bindex][i] = NULL;
			FERS_TotalAllocatedMem -= RX_BUFF_SIZE;
		}
	}
	return 0;
}


// --------------------------------------------------------------------------------------------------------- 
// Description: Enable/Disable data streaming
// Inputs:		bindex: board index
//				Enable: true/false
// Return:		0=OK, negative number = error code
// --------------------------------------------------------------------------------------------------------- 
int LLusb_StreamEnable(int bindex, bool Enable) {
	return FERS_usb[usbIdx[bindex]].stream_enable(Enable);
}

// --------------------------------------------------------------------------------------------------------- 
// Description: Reset IP address to default (192.168.50.3)
// Inputs:		bindex: board index
// Return:		0=OK, negative number = error code
// --------------------------------------------------------------------------------------------------------- 
int LLusb_Reset_IPaddress(int bindex) {
	return(FERS_usb[usbIdx[bindex]].set_service_reg(1, 0));
}

