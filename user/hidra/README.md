# HiDRA2
## Git workflow procedure

- Everyone works on its own branch
- When a new feature is ready it needs to be merged to master with a Pull Request:
   - Commit any relevant changes
   - `git switch master` and `git pull` to update local master branch
   - `git switch <your branch>` and `git rebase master` to include changes on master on your own branch
   - Solve any merge conflict and finally `git push` to push your changes to your remote branch
   - In the web browser go to pull request, select master as target branch and your branch as source and create the pull request
   - When request is ready to merge -----> merge it.


## Data format

### XDC 2025 event format

Decoder in [DRCalo/DreamDaqMon](https://github.com/DRCalo/DreamDaqMon)

TB data in [cernbox](https://cernbox.cern.ch/s/H6yDF4TNRez6jsw), or `/eos/user/i/ideadr/TB2025_H8`.

- **Event header**: 14 words, 32 bits each (in brackets fixed expected valuesd)
 ```
 eventMarker (0xccaaffee) | eventNumber | spillNumber |
 headerSize (0xe) | trailerSize (0x1) |  dataSize | eventSize (=header+trailer+data) |
 eventTimeSec | eventTimeMicrosec |
 triggerMask (0x1 or 0x2) | isPedMask | isPedFromScaler |
 sanityFlag | headerEndMarker (0xaccadead)
 ```
- **VME modules data** CAEN data format (QDCs V792, V792N; TDCs V775, V775N)
  - For each module: 1 header word + $n_chan$ data words + 1 trailer. 32 bits each word

- **Event trailer**: 1 word, 32 bits
 ```
 eventTrailer (0xbbeeddaa)
 ```








