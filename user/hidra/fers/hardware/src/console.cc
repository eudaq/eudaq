/******************************************************************************
* 
* CAEN SpA - Front End Division
* Via Vetraia, 11 - 55049 - Viareggio ITALY
* +390594388398 - www.caen.it
*
***************************************************************************//**
* \note TERMS OF USE:
* This program is free software; you can redistribute it and/or modify it under
* the terms of the GNU General Public License as published by the Free Software
* Foundation. This program is distributed in the hope that it will be useful, 
* but WITHOUT ANY WARRANTY; without even the implied warranty of 
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. The user relies on the 
* software, documentation and results solely at his own risk.
******************************************************************************/

//#include "MultiPlatform.h"
#include "console.h"


// *****************************************************************
// Global Variables
// *****************************************************************

f_socket_t ConSocket = 0;	// 0: stdio console; 1: console I/O through socket 
FILE *ConLog = NULL;
SocketBuffer_t Sbuff; 

#ifdef linux

#define CLEARSCR "clear"


/*****************************************************************************/
// kind of old, it is left since scanf is redefined as _scanf
static struct termios g_old_kbd_mode;

static void cooked(void)
{
	tcsetattr(STDIN_FILENO, TCSANOW, &g_old_kbd_mode);
}

static void raw(void)
{
	static char init=0;
	struct termios new_kbd_mode;

	if (init) return;
	/* put keyboard (stdin, actually) in raw, unbuffered mode */
	tcgetattr(0, &g_old_kbd_mode);
	memcpy(&new_kbd_mode, &g_old_kbd_mode, sizeof(struct termios));
	new_kbd_mode.c_lflag &= ~(ICANON | ECHO);
	new_kbd_mode.c_cc[VTIME] = 0;
	new_kbd_mode.c_cc[VMIN] = 1;
	tcsetattr(STDIN_FILENO, TCSANOW, &new_kbd_mode);
	/* when we exit, go back to normal, "cooked" mode */
	atexit(cooked);
	init = 1;
}

// --------------------------------------------------------------------------------------------------------- 
//  SCANF (change termios settings, then execute scanf) 
// --------------------------------------------------------------------------------------------------------- 
int _scanf(char *fmt, ...)	// // before calling the scanf function it is necessart to change termios settings
{
	int ret;
	//cooked();
	va_list args;
	va_start(args, fmt);
	ret = vscanf(fmt, args);
	va_end(args);
	//raw();
	return ret;
}

#else  // Windows

#include <windows.h>
#include <tchar.h>
#include <strsafe.h>

#endif

// --------------------------------------------------------------------------------------------------------
// --------------------------------------------------------------------------------------------------------
// --------------------------------------------------------------------------------------------------------
// --------------------------------------------------------------------------------------------------------
// --------------------------------------------------------------------------------------------------------
// FUNCTION OUT OF #IFDEF
// VALID FOR BOTH LINUX AND WINDOWS
// 
// PUT EVERYTHING DOWN HERE!!!!!!!!!
// --------------------------------------------------------------------------------------------------------

// Functions
void* ListenThread(void *arg) {
	int size;
	char *rxbuff;
	rxbuff = (char*)malloc(SOCKET_BUFFER_SIZE);

    // Receive until the peer closes the connection or an error occur in the socket
    do {
        size = recv(ConSocket, rxbuff, SOCKET_BUFFER_SIZE, 0);
        if (size > 0) {
			for(;;) {
				lock(Sbuff.mutex);
				if ((Sbuff.wpnt + size) < (2 * SOCKET_BUFFER_SIZE)) {
					memcpy(Sbuff.sharedData + Sbuff.wpnt, rxbuff, size); // put data in buffer...
					Sbuff.wpnt += size;
					unlock(Sbuff.mutex);
					break;
				}
				else {
					Sleep(10);
					unlock(Sbuff.mutex);
				}
			}
		}
		else if (size == 0) {
			Con_printf("L", "Console Connection lost\n");
		}
		else {
			Con_printf("L", "ERROR: recv failed with error: %d\n", f_socket_errno); // WSAGetLastError());
		}
    } while (size > 0);
	lock(Sbuff.mutex);
	memcpy(Sbuff.sharedData + Sbuff.wpnt, "q1", 2);
	Sbuff.wpnt += 2;
	unlock(Sbuff.mutex);

	free(rxbuff);
	return NULL;
}

int ConnectSocket()
{

	f_socket_t ConnectSocket = f_socket_invalid;
    struct addrinfo *result = NULL, *ptr = NULL, hints;
    int iResult;

	initmutex(Sbuff.mutex);  // CreateMutex(NULL, FALSE, NULL); // both?
 
#ifdef _WIN32
	WSADATA wsa;
	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {	// not needed for linux??
		Con_printf("L", "ERROR: Socket init failed. Error Code : %d", f_socket_errno); // WSAGetLastError());
		return -1;
	}
#endif

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_INET; // AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;

	// Resolve the server address and port
	iResult = getaddrinfo("localhost", DEFAULT_PORT, &hints, &result); // seems works in multiplatform
    if ( iResult != 0 ) {
		Con_printf("L", "ERROR: socket getaddrinfo error: %d", iResult);
#ifdef _WIN32
		WSACleanup();
#endif
		return -11;
	}

	// Attempt to connect to an address until one succeeds
	for (ptr = result; ptr != NULL; ptr = ptr->ai_next) {
		// Create a SOCKET for connecting to server
		ConnectSocket = socket(ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol);	// multiplatform?
		if (ConnectSocket == f_socket_invalid) {
			Con_printf("L", "ERROR: socket connect failed (error: %ld)\n", f_socket_errno); // WSAGetLastError());	// only windows
#ifdef _WIN32
			WSACleanup();
#endif
			return -1;
		}
		// Connect to server.
		iResult = connect(ConnectSocket, ptr->ai_addr, (int)ptr->ai_addrlen);	// seems multiplatform
		if (iResult == f_socket_error) {
#ifdef _WIN32
			closesocket(ConnectSocket);	// only windows
#else
			close(ConnectSocket);
#endif
			ConnectSocket = f_socket_invalid;
			continue;
		}
		break;
	}
	freeaddrinfo(result);
	if (ConnectSocket == f_socket_invalid) {
		Con_printf("L", "ERROR: Unable to connect to server!\n");
#ifdef _WIN32
		WSACleanup();
#endif
		return -1;
	}
	ConSocket = ConnectSocket;
	Con_printf("L", "Console Socket connected\n");

	// Start Listen thread
	//DWORD prodThrdID;
	//_beginthreadex(NULL, 0, (unsigned int(__stdcall *)(void*))ListenThread, 0, 0, (unsigned int *)&prodThrdID);
	f_thread_t prodThrdID;
	thread_create(ListenThread, NULL, &prodThrdID);
	return 0;
}


int CloseSocket()
{
	if (ConSocket == 0) return 0;
#ifdef _WIN32
	closesocket(ConSocket);
	WSACleanup();
#else
	close(ConSocket);
#endif
	Con_printf("L", "Console Socket closed\n");
	return 0;
}

int SocketGUIisempty()	// Fine??
{
	return (Sbuff.rpnt == Sbuff.wpnt);
}


int GetCharFromGUI() {	// Fine
	int c = 0;
	lock(Sbuff.mutex);
	if (Sbuff.rpnt < Sbuff.wpnt) {
		c = (int)Sbuff.sharedData[Sbuff.rpnt++];
		if (Sbuff.rpnt == Sbuff.wpnt) {  // buffer is empty
			Sbuff.wpnt = 0;
			Sbuff.rpnt = 0;
		}
	}
	unlock(Sbuff.mutex);
	return c;
}

int GetStringFromGUI(char* str, int* size, int max_size) {		
	int i = 0;

	*size = 0;
	lock(Sbuff.mutex);	
	while ((Sbuff.sharedData[Sbuff.rpnt] != '\n') && (Sbuff.rpnt < Sbuff.wpnt) && (i < max_size)) {
		str[i++] = Sbuff.sharedData[Sbuff.rpnt++];
	}
	*size = i;  // num of char + null terminator
	str[i] = 0;
	while (isspace(Sbuff.sharedData[Sbuff.rpnt]) && (Sbuff.rpnt < Sbuff.wpnt)) {
		Sbuff.rpnt++;
	}
	if (Sbuff.rpnt == Sbuff.wpnt) {  // buffer is empty
		Sbuff.wpnt = 0;
		Sbuff.rpnt = 0;
	}
	unlock(Sbuff.mutex);
	return 0;
}


// --------------------------------------------------------------------------------------------------------- 
//  Init console window (terminal)
// --------------------------------------------------------------------------------------------------------- 
int InitConsole(int Mode, FILE *log) 
{
	ConSocket = Mode & CONMODE_SOCKET;
	ConLog = log;
	if (!ConSocket) {
		
		
#ifdef _WIN32
		// Set console window size
		SMALL_RECT rect;
		COORD coord;
		coord.X = 150; // Defining our X and
		coord.Y = 50;  // Y size for buffer.

		rect.Top = 100;
		rect.Left = 0;
		rect.Bottom = coord.Y - 1; // height for window
		rect.Right = coord.X - 1;  // width for window

		//HANDLE hwnd = GetStdHandle(STD_OUTPUT_HANDLE); // get handle
		HANDLE hwnd = GetConsoleWindow();
		SetConsoleScreenBufferSize(hwnd, coord);       // set buffer size
		SetConsoleWindowInfo(hwnd, TRUE, &rect);       // set window size

		system("mode con: cols=80 lines=54");	// 
#endif
	}
	else {
		// open socket to GUI (this program is client, GUI is server)
		if (ConnectSocket() < 0) {
			return -1;
		}
	}
	return 0;
}

// --------------------------------------------------------------------------------------------------------- 
//  GETCH  
// --------------------------------------------------------------------------------------------------------- 
int Con_getch(void)
{
	if (!ConSocket) { // Took from CAENutility
#ifdef _WIN32
		return _getch();
#else
		struct termios oldattr;
		if (tcgetattr(STDIN_FILENO, &oldattr) == -1) perror(NULL);
		struct termios newattr = oldattr;
		newattr.c_lflag &= ~(ICANON | ECHO);
		newattr.c_cc[VTIME] = 0;
		newattr.c_cc[VMIN] = 1;
		if (tcsetattr(STDIN_FILENO, TCSANOW, &newattr) == -1) perror(NULL);
		const int ch = getchar();
		if (tcsetattr(STDIN_FILENO, TCSANOW, &oldattr) == -1) perror(NULL);
		return ch;
#endif
		//#ifdef _WIN32
		//		return _getch();
		//#else
		//		unsigned char temp;
		//
		//		raw();
		//		/* stdin = fd 0 */
		//		if (read(0, &temp, 1) != 1)
		//			return 0;
		//		return temp;
		//#endif
	}
	else {		
		while (SocketGUIisempty());
		return(GetCharFromGUI());
	}
}

// --------------------------------------------------------------------------------------------------------- 
//  KBHIT  
// --------------------------------------------------------------------------------------------------------- 
int Con_kbhit()
{
	if (!ConSocket) {
#ifdef _WIN32
		return _kbhit();
#else
		struct termios oldattr;
		if (tcgetattr(STDIN_FILENO, &oldattr) == -1) perror(NULL);
		struct termios newattr = oldattr;
		newattr.c_lflag &= ~(ICANON | ECHO);
		newattr.c_cc[VTIME] = 0;
		newattr.c_cc[VMIN] = 1;
		if (tcsetattr(STDIN_FILENO, TCSANOW, &newattr) == -1) perror(NULL);
		/* check stdin (fd 0) for activity */
		fd_set read_handles;
		FD_ZERO(&read_handles);
		FD_SET(0, &read_handles);
		struct timeval timeout;
		timeout.tv_sec = timeout.tv_usec = 0;
		int status = select(0 + 1, &read_handles, NULL, NULL, &timeout);
		if (tcsetattr(STDIN_FILENO, TCSANOW, &oldattr) == -1) perror(NULL);
		if (status < 0) {
			Con_printf("C", "select() failed in kbhit()\n");
			exit(1);
		}
		return status;
#endif
		//#ifdef _WIN32
		//		return _kbhit();
		//#else
		//		struct timeval timeout;
		//		fd_set read_handles;
		//		int status;
		//
		//		/* check stdin (fd 0) for activity */
		//		FD_ZERO(&read_handles);
		//		FD_SET(0, &read_handles);
		//		timeout.tv_sec = timeout.tv_usec = 0;
		//		status = select(0 + 1, &read_handles, NULL, NULL, &timeout);
		//		if (status < 0) {
		//			Con_printf("C", "select() failed in kbhit()\n");
		//			exit(1);
		//		}
		//		return (status);
		//#endif
	}
	else 		
		return !SocketGUIisempty();
}

// --------------------------------------------------------------------------------------------------------- 
// Description: get a string from stdin or socket (space are allowed)
// Input:		str: pointer to the string
//				MaxCount: max num of characters
// Return:		size of the string
// --------------------------------------------------------------------------------------------------------- 
int Con_GetString(char *str, int MaxCounts)
{
	if (!ConSocket) {
		fflush(stdin);
//#ifndef _WIN32
//		cooked();
//#endif
		fgets(str, MaxCounts, stdin);
//#ifndef _WIN32
//		raw();
//#endif
		return((int)strlen(str));
	}
	else {
		char data[SOCKET_BUFFER_SIZE];
		int size;
		if (GetStringFromGUI(data, &size, MaxCounts) < 0)
			return -1;
		strncpy(str, data, size);
		return size;
	}
}

// --------------------------------------------------------------------------------------------------------- 
// Description: get an integer from stdin or socket
// Input:		val: pointer to the int number
// Return:		0: OK, -1: error
// --------------------------------------------------------------------------------------------------------- 
int Con_GetInt(int *val)
{
	int ret = 0;
	if (!ConSocket) {
		if (myscanf("%d", val) != 1)	// scanf
			ret = -1;
	}
	else {
		char data[SOCKET_BUFFER_SIZE];
		int size;
		if (GetStringFromGUI(data, &size, SOCKET_BUFFER_SIZE) < 0)
			return -1;
		if (sscanf(data, "%d", val) != 1)
			ret = -1;
	}
	return ret;
}

// --------------------------------------------------------------------------------------------------------- 
// Description: clear the console
// --------------------------------------------------------------------------------------------------------- 
void ClearScreen()
{
	if (!ConSocket) {
		printf("\033[2J");
		gotoxy(0, 0);
	}

}

// --------------------------------------------------------------------------------------------------------- 
//  Set position for printf
// --------------------------------------------------------------------------------------------------------- 
void gotoxy(int x, int y)
{
	if (!ConSocket) printf("%c[%d;%df", 0x1B, y, x);
}

// --------------------------------------------------------------------------------------------------------- 
//  Send data to GUI
// --------------------------------------------------------------------------------------------------------- 
int SendDataToGUI(char* data, int size)
{
	//	c_send(const c_socket_t * sckt, const void* buffer, size_t totSize)
	if (send(ConSocket, data, size, 0) < 0) {	// send IS multiplatform!!!
		Con_printf("L", "ERROR: send data to socket failed\n");

#ifdef _WIN32
		WSACleanup();	// only on windows?
#endif
		return -1;
	}
	return 0;
}


// --------------------------------------------------------------------------------------------------------- 
// Description: printf to screen and to log file
// Return:		0: OK
// --------------------------------------------------------------------------------------------------------- 
int Con_printf(char *dest, char *fmt, ...) 
{
	const int msize = 2048;
	char msg[1000];
	uint16_t size;
	int8_t ret = 0;
	//static int cnt=0;
	va_list args;

	va_start(args, fmt);
	vsprintf(msg, fmt, args);
	va_end(args);

	if (!ConSocket && (strstr(dest, "C"))) { // Write to console
		if (strstr(dest, "w"))	// Warning msg in yellow
			printf(COLOR_YELLOW "%s" COLOR_RESET, msg);
		else if (strstr(dest, "e")) // Error message in Red
			printf(COLOR_RED "%s" COLOR_RESET, msg);
		else
			printf("%s", msg);
	}

	if (ConSocket && (strstr(dest, "S"))) {
		char buff[msize + 12], sdest[10];
		sprintf(sdest, "%s", strstr(dest, "S") + 1);
		size = 2 + (uint16_t)strlen(sdest) + (uint16_t)strlen(msg);
		buff[0] = size & 0xFF;
		buff[1] = (size >> 8) & 0xFF;
		//buff[2] = *(strstr(dest, "S")+1);  // destination of data packet in GUI program ('m' = log messsages, 's' = statistics, 'i' = info)
		memcpy(buff+2, sdest, sizeof(sdest));
		memcpy(buff+2+strlen(sdest), msg, sizeof(msg));
		SendDataToGUI(buff, size);
	}
		
	if ((ConLog != NULL) && (strstr(dest, "L"))) {
		static uint64_t t0=0;
		uint64_t elapsed_time;
		uint64_t log_time = j_get_time();
		if (t0 == 0) {
			char mytime[100];
			time_t startt;
			time(&startt);
			strcpy(mytime, asctime(gmtime(&startt)));
			mytime[strlen(mytime) - 1] = 0;
			fprintf(ConLog, "Starting Janus Log on %s UTC\n", mytime);
			t0 = log_time;
		}
		char type[50];
		char mmsg[500];

		elapsed_time = log_time - t0;
		uint64_t ms = elapsed_time % 1000;
		uint64_t s = (elapsed_time / 1000) % 60;
		uint64_t m = (elapsed_time / 60000) % 60;
		uint64_t h = (elapsed_time / 3600000);

		// warning: JW, error: JE, information: JI
		if (strstr(dest, "w"))	// Warning msg in yellow
			sprintf(type, "JW");	
		else if (strstr(dest, "e")) // Error message in Red
			sprintf(type, "JE");
		else
			sprintf(type, "JI");

		sprintf(mmsg, "[%02dh:%02dm:%02ds:%03dms][%s]%s", (int)h, (int)m, (int)s, (int)ms, type, msg);
		fprintf(ConLog, "%s", mmsg); // Write to Log File
		fflush(ConLog);
	}
	return 0;
}

