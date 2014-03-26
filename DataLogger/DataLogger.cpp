// DataLogger.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "Serial.h"
#include <iostream>

using namespace std;
int _tmain(int argc, _TCHAR* argv[])
{

	CSerial serial;
	if (serial.Open(2, 9600))
		cout << "Port opened successfully";
	else
		cout << "Failed to open port!";




	return 0;
}

