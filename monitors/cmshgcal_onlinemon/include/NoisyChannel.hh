#include <vector>
#include <iostream>

struct channelIdentifier
{
  channelIdentifier(std::string _rdbName,int _moduleID, int _chip, int _channel)
  {
    rdbName=_rdbName;
    moduleID=_moduleID;
    chip=_chip;
    channel=_channel;
  }
  std::string rdbName; // i.e HexaBoard-RDBID ID=0..15
  int moduleID;// 0..7 module id inside RD board
  int chip; //will be used as pixel_x for std plane
  int channel; //will be used as pixel_Y for std plane
};

inline bool operator==(const channelIdentifier &c1, const channelIdentifier &c2)
{
  return ( c1.rdbName==c2.rdbName &&
	   c1.moduleID==c2.moduleID &&
	   c1.chip==c2.chip &&
	   c1.channel==c2.channel );
}

class NoisyChannelList
{
public:
  NoisyChannelList(){;}
  void addNoisyChannel( channelIdentifier _chan ){ chans.push_back(_chan); }
  void addNoisyChannels( std::vector<channelIdentifier> _chans ){chans.insert( chans.end(),
									       _chans.begin(),
									       _chans.end() );}
  std::vector<channelIdentifier> get() const {
    return chans;
  }
private:
  std::vector<channelIdentifier> chans;
};

inline std::ostream &operator<<(std::ostream &os, const NoisyChannelList &list) {
  os << "List of noisy channels : RDBOARD | MODULE | CHIP | CHANNEL \n";
  for( std::vector<channelIdentifier>::const_iterator it=list.get().begin(); it!=list.get().end(); ++it )
    os << (*it).rdbName << " | " << (*it).moduleID << " | "<< (*it).chip << " | "<< (*it).channel << "\n";
  return os;
}

