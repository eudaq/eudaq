/*
	-----------------------------------------------------------------------------

	               --- CAEN SpA - Computing Systems Division --- 

	-----------------------------------------------------------------------------

	Name		:	CONSOLE.H

	Description :	Include file for 'Console.c'.
					
					Add '__LINUX' define to generate a Linux exe file.

					
	Date		:	November 2004
	Release		:	1.0
	Author		:	C.Landi



	-----------------------------------------------------------------------------

	This file contains the following procedures & functions declaration               
                                                                          
	con_init         initialize the console                                  
	con_end          close the console                                       
	write_log        write a message into the log file                       
	con_getch        get a char from console without echoing                 
	con_kbhit        read a char from console without stopping the program   
	con_scanf        read formatted data from the console                    
	con_printf       print formatted output to the standard output stream    
	gotoxy           set the cursor position                                 
	con_printf_xy    print formatted output on the X,Y screen position    
	clrscr           clear the screen                                        
	clear_line       clear a line                                            
	delay            wait n milliseconds 
	
	-----------------------------------------------------------------------------
*/


#ifndef __CONSOLE_H
#define __CONSOLE_H


//#define __LINUX


/****************************************************************************/
/*                                 short names                              */
/****************************************************************************/

#ifndef ulong
#define ulong unsigned long 
#endif
#ifndef uint
#define uint unsigned int 
#endif
#ifndef ushort
#define ushort unsigned short
#endif
#ifndef uchar
#define uchar unsigned char
#endif


/*******************************************************************************/
/*                       Usefull ascii codes                                   */
/*******************************************************************************/

#define CR           0x0D     /* carriage return */
#define BLANK        0x20     /* blank           */
#define ESC          0x1B     /* escape          */
#define BSP          0x08     /* back space      */

/* terminal types */
#define TERM_NOT_INIT   FALSE
#define TERM_INIT       TRUE


/****************************************************************************/
/*                              CON_INIT                                    */
/*--------------------------------------------------------------------------*/
/* Initialize the console                                                   */
/****************************************************************************/

void con_init(void);


/****************************************************************************/
/*                              CON_END                                     */
/*--------------------------------------------------------------------------*/
/* Reset the console to the default values                                  */
/****************************************************************************/

void con_end(void);


/******************************************************************************/
/*                             WRITE_LOG                                      */
/*----------------------------------------------------------------------------*/
/* parameters:     msg        ->   log message text                           */
/*----------------------------------------------------------------------------*/
/* Write a messege in the log file                                            */
/******************************************************************************/

void write_log(char *);


/******************************************************************************/
/*                               CON_GETCH                                    */
/*----------------------------------------------------------------------------*/
/* return:        c  ->  ascii code of pressed key                            */
/*----------------------------------------------------------------------------*/
/* Get a char from the console without echoing and return the character read. */
/******************************************************************************/

int  con_getch(void);


/******************************************************************************/
/*                               CON_KBHIT                                    */
/*----------------------------------------------------------------------------*/
/* return:        0  ->  no key pressed                                       */
/*                c  ->  ascii code of pressed key                            */
/*----------------------------------------------------------------------------*/
/* Rad the standard input buffer; if it is empty, return 0 else read and     */
/* return the ascii code of the first char in the buffer. A call to this      */
/* function doesn't stop the program if the input buffer is empty.            */
/******************************************************************************/

char con_kbhit(void);


/******************************************************************************/
/*                               CON_SCANF                                    */
/*----------------------------------------------------------------------------*/
/* parameters:     fmt       ->   format-control string                       */
/*                 app       ->   argument                                    */
/*----------------------------------------------------------------------------*/
/* return:         number of fields that were successfully converted and      */
/*                 assigned (0  ->  no fields were assigned)                  */
/*----------------------------------------------------------------------------*/
/* Rad formatted data from the console into the locations given by argument  */
/******************************************************************************/

int con_scanf(char *, void *);


/******************************************************************************/
/*                               CON_PRINTF                                   */
/*----------------------------------------------------------------------------*/
/* parameters:     fmt       ->   format string: must contain specifications  */
/*                                that determine the output format for the    */
/*                                argument                                    */
/*----------------------------------------------------------------------------*/
/* return:         number of characters printed                               */
/*                 or a negative value if an error occurs                     */
/*----------------------------------------------------------------------------*/
/* Print formatted output to the standard output stream                       */ 
/******************************************************************************/

int con_printf(char *,...);


/****************************************************************************/
/*                               GOTOXY                                     */
/*--------------------------------------------------------------------------*/
/* parameters:     x, y ->   position on the screen                         */ 
/*--------------------------------------------------------------------------*/
/* Place the cursor at position x,y on the screen                           */
/****************************************************************************/

void gotoxy(int, int);


/****************************************************************************/
/*                             CON_PRINTF_XY                                */
/*--------------------------------------------------------------------------*/
/* parameters:     xpos, ypos ->   position on the screen                   */ 
/*                 msg        ->   message text                             */
/*--------------------------------------------------------------------------*/
/* Print a messege on the X,Y screen position                               */
/****************************************************************************/
 
int con_printf_xy(uint, uint, char *,...);


/****************************************************************************/
/*                               CLRSCR                                     */
/*--------------------------------------------------------------------------*/
/* Clear the screen                                                         */
/****************************************************************************/

void clrscr(void);


/****************************************************************************/
/*                              CLEAR_LINE                                  */
/*--------------------------------------------------------------------------*/
/* parameters:     line ->   line to clear                                  */ 
/*--------------------------------------------------------------------------*/
/* Clear a line of the screen                                               */
/****************************************************************************/

void clear_line(uint);


/****************************************************************************/
/*                             DELAY                                        */
/*--------------------------------------------------------------------------*/
/* parameters:     del        ->   delay in millisecond                     */ 
/*--------------------------------------------------------------------------*/
/* Wait n milliseconds                                                      */
/****************************************************************************/

void delay(int);


#endif

