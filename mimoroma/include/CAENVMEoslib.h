/***************************************************************************/
/*                                                                         */
/*               --- CAEN SpA - Computing Systems Division ---             */
/*                                                                         */
/*    V1718Lib.H                                            	     	   */
/*                                                                         */
/*                                                                         */ 
/*    Created: January 2004                                                */
/*                                                                         */
/*                                                                         */
/*                                                                         */
/***************************************************************************/

#ifndef __V1718OSLIB_H
#define __V1718OSLIB_H

#ifdef WIN32

#include <windows.h>
#include <winioctl.h>

// Con il file .def non dovrebbe servire il dllexport
//#ifdef CAENVME
//#define CAENVME_API __declspec(dllexport) CVErrorCodes __stdcall
//#else
#define CAENVME_API CVErrorCodes __stdcall
//#endif

#else   // WIN32

#define CAENVME_API CVErrorCodes

#endif  // WIN32

#endif  // V1718OSLIB_H
