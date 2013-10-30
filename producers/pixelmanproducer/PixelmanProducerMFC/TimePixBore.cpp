#include "stdafx.h"
#include "TimePixBore.h"
#include "medipixChipId.h"

TimePixBore::TimePixBore(medipixChipId *param, double timeToEndOfShutter)
{
	version = VERSION;
	m_rowLen = param->deviceInfo.rowLen;                  
	m_numberOfRows = param->deviceInfo.numberOfRows;
	m_mpxType = param->deviceInfo.mpxType;                       
	m_chipboardID[MPX_MAX_CHBID] = param->deviceInfo.chipboardID[MPX_MAX_CHBID];   
	m_ifaceName = param->deviceInfo.ifaceName;  
	m_clockTimepix = param->deviceInfo.clockTimepix;
}