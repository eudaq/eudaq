#include "prod.h"

#include <windows.h>
#include "TSystem.h"


int main(){



	
		auto a=new prod();
		a->Connect2run("127.0.0.1:44000");

		gSystem->Sleep(10000000);

}