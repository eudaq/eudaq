#include "prod.h"
#include "..\RootInterface\inc\SCTProducer.h"
#include <iostream>
#include <vector>

prod::prod():a(0) {

	//a=new SCTProducer("SCTupgrade","192.168.0.11:44000");
	//a->Connect("send_start_run()","prod",this,"readoutloop()");
}

prod::~prod(){
	delete a;
}

void prod::Connect2run(const char *name){
	a=new SCTProducer("SCTupgrade",name);
	a->Connect("send_start_run()","prod",this,"readoutloop()");
	a->Connect("send_onConfigure()","prod",this,"onConfigure()");
	a->Connect("send_onStop()","prod",this,"onStopping()");
}

void  prod::readoutloop(){
	isRunning=true;
	while(isRunning){
		sendEvent();
	//	gSystem->Sleep(10000);
	}
	std::cout<<"prod::readoutloop() has stopt"<<std::endl;
}
void prod::onConfigure(){
	std::cout<<"ConfigurationSatus() =" <<a->ConfigurationSatus()<<std::endl;
	// std::cout<<"a->getConfiguration_TString(\"Parameter\", \"nix\") =" <<a->getConfiguration("Parameter", 1)<<std::endl;
	char buffer[20];

	std::cout<<a->getConfiguration( "BitFile", "nix",buffer,20 )<<std::endl;
	std::cout<<buffer<<std::endl;
}
void  prod::onStopping(){
	std::cout<<"prod::onStopping()"<<std::endl;
	isRunning=false;
	gSystem->Sleep(10000);
}
void prod::sendEvent(){

	a->createNewEvent();
	std::vector<unsigned char> vec;
	vec.push_back('h');
	vec.push_back('a');
	vec.push_back('l');
	vec.push_back('l');
	vec.push_back('o');
	a->AddPlane2Event(0,vec);

	std::vector<unsigned char> vec1;
	vec1.push_back('w');
	vec1.push_back('e');
	vec1.push_back('l');
	vec1.push_back('t');

	a->AddPlane2Event(1,vec1);
	a->sendEvent();
}