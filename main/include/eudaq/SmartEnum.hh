/*
 * SmartEnum.hh
 *
 *  Created on: Jun 27, 2014
 *      Author: behrens
 */

#ifndef SMARTENUM_HH_
#define SMARTENUM_HH_

class SmartEnumBase {
};

#define DECLARE_ENUM_CLASS(ClassName, ...)\
class ClassName : public SmartEnumBase {\
  public:\
    enum Value { __VA_ARGS__ } value;\
    const std::string& toString() { return toString( value ); }\
    static const std::string& toString( Value& v ) {\
  	  return getMap()[v]; \
    }\
  private:\
    static std::map<Value,std::string>& getMap() {\
      static const std::string str[] = { #__VA_ARGS__ };\
      static const Value val[] = {__VA_ARGS__};\
      static std::map<Value,std::string> map; \
      if ( map.empty() )\
        for ( int i = 0; i < sizeof(val) / sizeof(Value); i++ )\
          map[val[i]] = str[i];\
      return map;\
    }\
};



#endif /* SMARTENUM_HH_ */
