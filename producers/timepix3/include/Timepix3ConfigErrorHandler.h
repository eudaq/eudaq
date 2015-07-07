#ifndef TIMEPIX3CONFIGERRORHANDLER_H
#define TIMEPIX3CONFIGERRORHANDLER_H
#include <xercesc/util/XMLString.hpp>
#include <xercesc/sax/ErrorHandler.hpp>
#include <xercesc/sax/SAXParseException.hpp>
#include <iostream>

class Timepix3ConfigErrorHandler: xercesc::ErrorHandler {
 public:
  virtual void error(const xercesc::SAXParseException& exc);
  virtual void fatalError(const xercesc::SAXParseException& exc);
  virtual void warning(const xercesc::SAXParseException& exc);
  virtual void resetErrors();
};

inline void Timepix3ConfigErrorHandler::resetErrors()
{
}

inline void Timepix3ConfigErrorHandler::warning(const xercesc::SAXParseException& exc)
{
  char* message = xercesc::XMLString::transcode( exc.getMessage() );
  std::cout << "Warning:" << message << std::endl;
  throw exc;
}

inline void Timepix3ConfigErrorHandler::error(const xercesc::SAXParseException& exc)
{
  char* message = xercesc::XMLString::transcode( exc.getMessage() );
  std::cout << "Error:" << message << std::endl;
  throw exc;
}

inline void Timepix3ConfigErrorHandler::fatalError(const xercesc::SAXParseException& exc)
{
  throw exc;
}

#endif
