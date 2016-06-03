
/**
 * IRC exception classes
 */

#ifndef IRC_EXCEPTIONS_H
#define IRC_EXCEPTIONS_H

#include <exception>
#include <string>

namespace irc {

  /** Base class for all exceptions thrown by the irc framework.
   */
  class ircException : public eudaq::LoggedException {
  public:
    ircException(const std::string& what_arg) : eudaq::LoggedException(what_arg),ErrorMessage(what_arg) {}
    ~ircException() throw() {};
    virtual const char* what() const throw(){
      return ErrorMessage.c_str();
    };
  private:
    std::string ErrorMessage;
  };

  /**  This class of exceptions covers issues with the configuration found during runtime:
   *    - out-of-range parameters
   *    - missing (crucial) parameters
   *    - inconsistent or mismatched configuration sets
   */
  class InvalidConfig : public ircException {
  public:
    InvalidConfig(const std::string& what_arg) : ircException(what_arg) {}
  };

  // not used
  /**  This exception class covers issues with a DTB firmware version
   *   mismatch (i.e. between the RPC interfaces of irc and the NIOS code).
   */
  //class FirmwareVersionMismatch : public ircException {
  //public:
  //  FirmwareVersionMismatch(const std::string& what_arg) : ircException(what_arg) {}
  //};

  /**  This exception class covers connection issues with the shared memory
   *   which is allocated by the LV DAQ
   */
  class GlobalSharedMemError : public ircException {
  public:
    GlobalSharedMemError(const std::string& what_arg) : ircException(what_arg) {}
  };


  /**  This exception class covers global initialisation error
   *   e.g. ??
   */
  class GlobalConnectError : public ircException {
  public:
    GlobalConnectError(const std::string& what_arg) : ircException(what_arg) {}
  };

  // not used
  /**  This exception class is used for timeouts occuring during USB readout.
   */
  //class UsbConnectionTimeout : public ircException {
  //public:
  //  UsbConnectionTimeout(const std::string& what_arg) : ircException(what_arg) {}
  //};

 /** This exception class is the base class for all send/receive/execute cmd error
   */
  class ComException : public ircException {
  public:
    ComException(const std::string& what_arg) : ircException(what_arg) {}
  };

 /** This exception class covers all send/receive cmd errors
   */
  class SendReceiveException : public ComException {
  public:
    SendReceiveException(const std::string& what_arg) : ComException(what_arg) {}
  };

 /** This exception class covers all execute cmd errors
   */
  class ExecuteException : public ComException {
  public:
    ExecuteException(const std::string& what_arg) : ComException(what_arg) {}
  };



 /** This exception class is the base class for all irc data exceptions
   */
  class DataException : public ircException {
  public:
    DataException(const std::string& what_arg) : ircException(what_arg) {}
  };

  /** This exception class is used in case a new event is requested but nothing available. Usually
   *  this is not critical and should be caught by the caller. E.g. when runninng a DAQ with
   *  external triggering and constant event polling from the DTB it can not be ensured that data
   *  is always available, but returning an empty event will mess up trigger sync.
   */
  class DataNoEvent : public DataException {
  public:
    DataNoEvent(const std::string& what_arg) : DataException(what_arg) {}
  };

  /** This exception class is used whenever multiple DAQ channels are active and
   *  there is a mismatch in event number across the channels (i.e. channel 0
   *  still returns one event but channel 1 is already drained
   */
  class DataChannelMismatch : public DataException {
  public:
    DataChannelMismatch(const std::string& what_arg) : DataException(what_arg) {}
  };

  /**  This exception class is used when the DAQ readout is incomplete (missing events).
   */
  class DataMissingEvent : public DataException {
  public:
    uint32_t numberMissing;
    DataMissingEvent(const std::string& what_arg, uint32_t nmiss) : DataException(what_arg), numberMissing(nmiss) {}
  };

  /**  This exception class is used when raw Mimosa values could not be decoded
   */
  class DataDecodingError : public DataException {
  public:
    DataDecodingError(const std::string& what_arg) : DataException(what_arg) {}
  };

  // not used
  /** This exception class is used when the DESER400 module reports a data handling error.
   */
  //class DataDeserializerError : public DataDecodingError {
  //public:
  //  DataDeserializerError(const std::string& what_arg) : DataDecodingError(what_arg) {}
  //};

  // not used
  /** This exception class is used when out-of-range pixel addresses
   *  are found during the decoding of the raw values.
   */
  //class DataInvalidAddressError : public DataDecodingError {
  //public:
  //  DataInvalidAddressError(const std::string& what_arg) : DataDecodingError(what_arg) {}
  //};

  // not used
  /** This exception class is used when the pulse-height fill-bit (dividing the eight bits
   *  into two blocks of four bits) is not zero as it should.
   */
  //class DataInvalidPulseheightError : public DataDecodingError {
  //public:
  //  DataInvalidPulseheightError(const std::string& what_arg) : DataDecodingError(what_arg) {}
  //};

  // not used
  /** This exception class is used when the decoded pixel address returns row == 80, which
   *  points to corrupt data buffer rather than invalid address.
   */
  //class DataCorruptBufferError : public DataDecodingError {
  //public:
  //  DataCorruptBufferError(const std::string& what_arg) : DataDecodingError(what_arg) {}
  //};
  
} //namespace irc

#endif /* IRC_EXCEPTIONS_H */

