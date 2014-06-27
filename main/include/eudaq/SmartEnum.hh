/*
 * SmartEnum.hh
 *
 *  Created on: Jun 27, 2014
 *      Author: behrens
 */

#ifndef SMARTENUM_HH_
#define SMARTENUM_HH_


class SmartEnum {
	int value;
	char const * name ;
  protected:
    SmartEnum( int v, char const * n ) : value(v), name(n) {}
  public:
    int asInt() const { return value ; }
    char const * cstr() { return name ; }
};



#endif /* SMARTENUM_HH_ */
