//extends the CEdit class to provide easy access to generic data types, e.g getInt, getDouble, getStdStr

#pragma once
#include <string>


class CEditExtended : public CEdit 
{
public:
	CEditExtended();		
	~CEditExtended();


	CString getCStr();
	std::string getStdStr();
	int getInt();
	double getDouble();

private:
	CString cstrBuffer;
	std::string stdStrBuffer;
	int intBuffer;
	double doubleBuffer;
	
};

