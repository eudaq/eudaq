#include "eudaq/FileWriterNull.hh"

namespace eudaq {

  namespace {
    RegisterFileWriter<FileWriterNull> reg("NULL");
  }

}
