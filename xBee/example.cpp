#include <iostream>
#include <string>
#include <unistd.h>
#include "xBee.h"

using namespace std;
int main(int argc, char** argv){

	xBee xbee;
	int port = -1;
	int option = -1;
	string someString;

	
	
	while (option != 1 && option != 0){

		cout << "Please select read or write mode (0/1)" << endl;
		cin >> option;





	}


	try {
		port = xbee.init();
	}
	catch (const char* exception) {
		cout << exception << endl;
	}

	
	if (port != -1){

		cout << "Connection successful." << endl;
		
		

	}else{

		cout << "Connection failed!" << endl;

	}

	if (option == 1){

		while(true){

			cout << "Please enter a message" << endl;
			cin >> someString;
			xbee.printInput(someString, port);
			cout << someString + " printed to xBee reciever" << endl;



		}


		


	}else if (option == 0){


		//int tics = 10;
		string outPut = "0";

		while (true){

		outPut = xbee.readOutput(port);
		cout << outPut << endl;
		//tics--;
		usleep(1000000);


		}



	}

	
	

	
	

	
	cout << "Done" << endl;








}