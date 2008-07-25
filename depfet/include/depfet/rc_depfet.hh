
#include <string>
#include <list>
#include <iostream>
#include <sstream>
#include <sys/types.h>
#include <sys/socket.h>

int cmd_send(const std::string & command);
int set_host(char *host, int port);

using namespace std;
const int LHOST=128;
struct HOST {
    int PORT;
    char NAME[LHOST];
    int sock;
};

//----------------------------------------------------------------

