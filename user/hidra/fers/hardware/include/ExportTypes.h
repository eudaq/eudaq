/******************************************************************************
*
* CAEN SpA - Front End Division
* Via Vetraia, 11 - 55049 - Viareggio ITALY
* +390594388398 - www.caen.it
*
*******************************************************************************
* \note TERMS OF USE:
* This program is free software; you can redistribute it and/or modify it under
* the terms of the GNU General Public License as published by the Free Software
* Foundation. This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. The user relies on the
* software, documentation and results solely at his own risk.
******************************************************************************/

#ifndef __EXPORTTYPES_H
#define __EXPORTTYPES_H

#ifdef __cplusplus
extern "C" {
#endif

#ifdef _WIN32
#ifdef _CAEN_FERS_EXPORT
#define CAEN_FERS_DLLAPI __declspec(dllexport)
#else
#define CAEN_FERS_DLLAPI __declspec(dllimport)
#endif
#else
#define CAEN_FERS_API
#define CAEN_FERS_DLLAPI __attribute__((visibility("default")))
#endif

#ifdef __cplusplus
}
#endif

#endif