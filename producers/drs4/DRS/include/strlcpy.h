/********************************************************************\

   Name:         strlcpy.h
   Created by:   Stefan Ritt
   Copyright 2000 + Stefan Ritt

   Contents:     Header file for strlcpy.c
   
   This file is part of MIDAS XML Library.

   MIDAS XML Library is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   MIDAS XML Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with MIDAS XML Library.  If not, see <http://www.gnu.org/licenses/>.

\********************************************************************/

#ifndef _STRLCPY_H_
#define _STRLCPY_H_

// some version of gcc have a built-in strlcpy
#ifdef strlcpy
#define STRLCPY_DEFINED
#endif

#ifdef __cplusplus
extern "C" {
#endif

#ifndef EXPRT
#if defined(EXPORT_DLL)
#define EXPRT __declspec(dllexport)
#else
#define EXPRT
#endif
#endif

#ifndef STRLCPY_DEFINED
size_t EXPRT strlcpy(char *dst, const char *src, size_t size);
size_t EXPRT strlcat(char *dst, const char *src, size_t size);
#endif

#ifdef __cplusplus
}
#endif

#endif /*_STRLCPY_H_ */
