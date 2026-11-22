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

#ifndef _CONSOLE_H
#define _CONSOLE_H

#include <stdio.h>
#include "MultiPlatform.h"

#define COLOR_BLACK				"\x1b[30m"
#define COLOR_RED				"\x1b[31m"
#define COLOR_GREEN				"\x1b[32m"
#define COLOR_YELLOW			"\x1b[33m"
#define COLOR_BLUE				"\x1b[34m"
#define COLOR_MAGENTA			"\x1b[35m"
#define COLOR_CYAN				"\x1b[36m"
#define COLOR_WHITE				"\x1b[37m"

#define BKG_COLOR_BLACK			"\x1b[40m"
#define BKG_COLOR_RED			"\x1b[41m"
#define BKG_COLOR_GREEN			"\x1b[42m"
#define BKG_COLOR_YELLOW		"\x1b[43m"
#define BKG_COLOR_BLUE			"\x1b[44m"
#define BKG_COLOR_MAGENTA		"\x1b[45m"
#define BKG_COLOR_CYAN			"\x1b[46m"
#define BKG_COLOR_WHITE			"\x1b[47m"

#define COLOR_RESET				"\x1b[0m"
//#define COLOR_RESET				COLOR_BLACK	

#define FONT_STYLE_BOLD			"\x1B[1m"
#define FONT_STYLE_FAINT		"\x1B[2m"
#define FONT_STYLE_ITALIC		"\x1B[3m"
#define FONT_STYLE_UNDERLINED	"\x1B[4m"
#define FONT_STYLE_INVERSE		"\x1B[7m"
#define FONT_STYLE_STRIKE		"\x1B[9m"


#define CONMODE_SOCKET		0x0001

#define DEFAULT_PORT "50007"
 
#define SOCKET_BUFFER_SIZE 1024



/* buffer using a shared integer variable */
typedef struct {
	mutex_t mutex;  // to guard the buffer
	char sharedData[2*SOCKET_BUFFER_SIZE]; // pay-load
	int wpnt, rpnt; // pointers
} SocketBuffer_t;


#ifdef linux
    #include <sys/time.h> /* struct timeval, select() */
    #include <termios.h>  /* tcgetattr(), tcsetattr() */
    #include <stdlib.h>   /* atexit(), exit() */
    #include <unistd.h>   /* read() */
    #include <string.h>   /* memcpy() */
    #include <stdint.h>
    #include <sys/stat.h>

    #define myscanf     _scanf  // before calling the scanf function it is necessart to change termios settings
	#define getch       Con_getch
	#define kbhit       Con_kbhit

//int getch(void);
//int kbhit();
int _scanf(char *fmt, ...);


#else  // Windows

    #pragma comment(lib, "ws2_32.lib") // Winsock Library

    #include <conio.h>
    #include <process.h>
	#include <stdint.h>
	#include <string.h>   

    #define myscanf     scanf
	#define getch       Con_getch
	#define kbhit       Con_kbhit
	
#endif


//****************************************************************************
// Function prototypes
//****************************************************************************
int InitConsole(int Mode, FILE *log);
int Con_kbhit();
int Con_getch();
int Con_GetString(char *str, int MaxCounts);
int Con_GetInt(int *val);
void ClearScreen();
void gotoxy(int x,int y);
int Con_printf(char *dest, char *fmt, ...);


#endif
