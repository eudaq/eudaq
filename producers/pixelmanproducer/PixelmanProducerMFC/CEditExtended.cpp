#include "stdafx.h"
#include "CEditExtended.h"


CEditExtended::CEditExtended():CEdit()
{

}

CEditExtended::~CEditExtended()
{

}

CString CEditExtended::getCStr()
{
	cstrBuffer = "";
	this->GetWindowText(cstrBuffer);
	return cstrBuffer;
}


std::string CEditExtended::getStdStr()
{	
	
	cstrBuffer = "";
	this->GetWindowText(cstrBuffer);
	CT2CA pszConvertedAnsiString(cstrBuffer);//Klasse die von CString zu std::String casted
	//return pszConvertedAnsiString;
	return (std::string) pszConvertedAnsiString;

	
}

int CEditExtended::getInt()
{
	memset(&intBuffer, 0, sizeof(intBuffer));
	this->GetWindowText(cstrBuffer);
	intBuffer = atoi(cstrBuffer);
	return intBuffer;
}

double CEditExtended::getDouble()
{
	memset(&doubleBuffer, 0, sizeof(doubleBuffer));
	cstrBuffer = "";
	this->GetWindowText(cstrBuffer);
	doubleBuffer = atof(cstrBuffer);
	return doubleBuffer;
}