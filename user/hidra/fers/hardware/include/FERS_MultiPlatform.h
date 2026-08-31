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

#ifndef _FERSMULTIPLATFORM_H
#define _FERSMULTIPLATFORM_H

#include <stdint.h>
#include <ctype.h>    /* toupper() */   /*isspace()*/
#include <string.h>
#include <stdbool.h>
#include <stdarg.h>
#include <inttypes.h>

#ifndef INT32_C
#define INT32_C(x)   (x)
#endif

// shutdown the socket before closing
#define SHUT_SEND	1	// Windows SD_SEND, Linux SHUT_WR, both = 1
#define SHUT_BOTH	2	// Windows SD_BOTH, Linux SHUT_RDWR, both = 2

#ifdef _WIN32
	#define _WINSOCKAPI_	// stops windows.h including winsock.h
	#include <WinSock2.h>
	//#include <winsock.h>
	#include <WS2tcpip.h>
	#include <Windows.h>
	#include <process.h>
	#include <direct.h>
	#include <io.h>
	#include <time.h>
	#include <sys/timeb.h>
	#include <sys\stat.h>

	#define access(path, amode)			_access(path, amode)

#else
	#include <pthread.h>
	#include <time.h>
	#include <sys/time.h>
	#include <sys/stat.h>

	#include <sys/types.h>
	#include <sys/socket.h>	// send receive
	#include <netinet/in.h>
	#include <arpa/inet.h>
	#include <netdb.h>
	#include <unistd.h>
	#include <errno.h>
	#include <semaphore.h>

	#include <endian.h>
#endif

#ifndef INFINITE
//#define INFINITE		INT32_C(-1)
constexpr int INFINITE = -1;
#endif

// Socket definition
#ifdef linux
typedef int							f_socket_t;			//!< Return type of socket(). On Linux socket() returns int.
#define f_socket_errno				errno				//!< On Linux socket-related functions set the error into errno variable.
#define f_socket_h_errno			h_errno				//!< On Linux network database operations like gethostbyname() set the error into h_errno variable.
#define f_socket_invalid			(-1)				//!< On Windows functions like accept() return INVALID_SOCKET in case of error. On Linux they return -1.
#define f_socket_error				(-1)				//!< On Windows functions like send() return SOCKET_ERROR in case of error. On Linux they return -1.
#define f_socket_close(f_sock)		close(f_sock)		//!< On Windows closesocket. On linux close>
#define f_socket_cleanup()								//!< On Windows WSACleanup. On linux "do nothing">

#define j_strdup					strdup
#else
typedef SOCKET						f_socket_t;			//!< Return type of socket(). On Windows socket() returns SOCKET.
typedef int							ssize_t;			//!< Used on Linux as return type of send() an recv(). On Windows they return int.
#define f_socket_errno				WSAGetLastError()	//!< On Windows socket-related functions set an error retrievable from WSAGetLastError()
#define f_socket_h_errno			f_socket_errno		//!< On Windows network database operations like gethostbyname() set the error into WSAGetLastError() too.
#define f_socket_invalid			INVALID_SOCKET		//!< On Windows functions like accept() return INVALID_SOCKET in case of error. On Linux they return -1.
#define f_socket_error				SOCKET_ERROR		//!< On Windows functions like send() return SOCKET_ERROR in case of error. On Linux they return -1.
#define f_socket_close(f_sock)		closesocket(f_sock)	//!< On Windows closesocket. On linux close>
#define f_socket_cleanup()			WSACleanup()		//!< On Windows WSACleanup. On linux "do nothing">

#define j_strdup						_strdup
#endif // linux

// Thread??
#ifdef _WIN32
	typedef HANDLE                  mutex_t;
	typedef int						f_thread_t;
	typedef HANDLE					f_sem_t;
	#define initmutex(m)            (m = CreateMutex(NULL, FALSE, NULL))==NULL ? GetLastError() : 0
	#define destroymutex(m)         ReleaseMutex(m) != FALSE ? 0 : GetLastError()
	#define lock(m)                 (WaitForSingleObject(m, INFINITE) == WAIT_FAILED) ? GetLastError() : 0
	#define unlock(m)               (ReleaseMutex(m) != 0) ? 0 : GetLastError()
	#define trylock(m)           	WaitForSingleObject(m, 10)

	#define thread_create(f, p, id)	_beginthreadex(NULL, 0, (unsigned int(__stdcall *)(void*))f, p, 0, (unsigned int *)id);
	#define thread_join(id, r)		WaitForSingleObject((HANDLE *)id, INFINITE)

#ifdef __cplusplus
extern "C" {
#endif
	uint64_t get_time();
#ifdef __cplusplus
}
#endif

#endif

#ifdef linux

	// LINUX VERSION NOT TESTED!!!
	typedef pthread_mutex_t			mutex_t;
	typedef pthread_t				f_thread_t;
	typedef sem_t					f_sem_t;
	#define initmutex(m)			pthread_mutex_init(&m, NULL)
//	#define initmutex(m)			(m = PTHREAD_MUTEX_INITIALIZER)
	#define destroymutex(m)			pthread_mutex_destroy(&m)
	#define lock(m)					pthread_mutex_lock(&m)
	#define unlock(m)				pthread_mutex_unlock(&m)
	#define trylock(m)				pthread_mutex_trylock(&m) // pthread_mutex_timedlock(&m, t)

	#define thread_create(f, p, id)	pthread_create(id, NULL, f, p)
	#define thread_join(id, r)		pthread_join(id, r)

	#define Sleep(ms)				usleep((ms)*1000) // DNIN : usleep is already in ms?? seems no

#ifdef __cplusplus
extern "C" {
#endif
	uint64_t get_time();
#ifdef __cplusplus
}
#endif

#endif

//// Structure for mutex_t
//typedef struct {
//	mutex_t mutex;
//	int mutex_init;
//} f_mutex_t;


// TAKEN FROM CAENUtility

int GetFileUpdateTime(char *fname, uint64_t *ftime);
#ifdef __cplusplus
extern "C" {
#endif
	int f_sem_init(f_sem_t* sem);
	int f_sem_destroy(f_sem_t* sem);
	int f_sem_wait(f_sem_t* sem, int32_t ms);
	int f_sem_post(f_sem_t* sem);

	//int f_mutex_init(f_mutex_t* mutex);
	//int f_mutex_destroy(f_mutex_t* mutex);
#ifdef __cplusplus
}
#endif

#endif
