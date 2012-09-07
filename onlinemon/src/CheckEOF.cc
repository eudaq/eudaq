#include "CheckEOF.hh"
#include <TFile.h>

void CheckEOF::setCollections(std::vector <BaseCollection*> colls) {
		_colls = colls;
}
	
void CheckEOF::TimerLoop() {
	if (_isEOF) {
		TFile *f = new TFile(_rootfilename.c_str(),"RECREATE");
		for (unsigned int i = 0 ; i < _colls.size(); ++i) {
			_colls.at(i)->Write(f);
		}
		f->Close(); 
		exit(0); // Stop the program
	} else _isEOF = true; // let's see if it's still true after another 10 seconds
}

CheckEOF::CheckEOF() : _isEOF(false) {
		timer = new TTimer();
		timer->Connect("Timeout()","CheckEOF",this,"TimerLoop()");
		
	}
	
void CheckEOF::startChecking(int mtime) {
	timer->Start(mtime,kFALSE);
}

