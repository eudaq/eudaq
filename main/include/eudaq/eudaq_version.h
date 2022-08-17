#ifndef EUDAQ_VERSION_H
#define EUDAQ_VERSION_H

#include <string>

namespace eudaq {
namespace version {
    std::string getEudaqVersion();
    unsigned getEudaqVersionMajor();
    unsigned getEudaqVersionMinor();
    unsigned getEudaqVersionPatch();
    unsigned getEudaqVersionTweak();

    std::string getEudaqGitBranch();
    std::string getEudaqGitTag();
    std::string getEudaqGitHash();
    std::string getEudaqGitDate();
    std::string getEudaqGitSubject();
}
}
#endif
