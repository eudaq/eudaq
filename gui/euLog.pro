TARGET = euLog
include(common.pro)

# Input
HEADERS +=include/LogCollectorModel.hh LogDialog.hh
SOURCES += src/LogCollectorModel.cc
FORMS += ui/LogDialog.ui
