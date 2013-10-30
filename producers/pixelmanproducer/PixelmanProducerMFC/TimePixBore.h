#pragma once
#define VERSION 0.0

#include "medipixChipId.h"

class TimePixBore
{
public:
	TimePixBore(medipixChipId *param, double timeToEndOfShutter);
	//virtual ~TimePixBore();
	unsigned int version;
	int m_rowLen;                   // number of chips
    int m_numberOfRows;                   // number of rows in which chips are aligned (e.g. quad has 4 chips, which are in 2 rows)
    int m_mpxType;                        // medipix type - MPX_ORIG, MPX_MXR, MPX_TPX
    char m_chipboardID[MPX_MAX_CHBID];    // id of chip/chipboard
	const char *m_ifaceName;  
	double m_clockTimepix;
	double timeToEndOfShutter;

protected:
	medipixChipId deviceInfos;

};