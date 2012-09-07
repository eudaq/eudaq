
#ifndef SIMPLESTANDARDPLANEDOUBLE_HH_
#define SIMPLESTANDARDPLANEDOUBLE_HH_



#include <string>
#include <map>
#include "SimpleStandardPlane.hh"

class SimpleStandardPlaneDouble {
protected:
	SimpleStandardPlane _p1;
	SimpleStandardPlane _p2;
public:
	SimpleStandardPlaneDouble(const SimpleStandardPlane &p1, const SimpleStandardPlane &p2) : _p1(p1), _p2(p2) {}
	std::string getName() const {
		char out[1024];
		sprintf(out, "%s%i_vs_%s%i",_p1.getName().c_str(),_p1.getID(),_p2.getName().c_str(),_p2.getID());
		return std::string(out);
	}
	SimpleStandardPlane getPlane1() const { return _p1; }
	SimpleStandardPlane getPlane2() const { return _p2; }
};


#endif
