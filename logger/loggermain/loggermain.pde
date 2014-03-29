PrintWriter outputLog;
Serial myPort;  // The serial port:
	 	boolean printThisLine;
	int newline;
	char currentChar;
import processing.serial.*;


final int MAX_CHAR_SIZE = 500000;

void setup() {

	int mo = month();
	int day = day();
	int sec = second();  // Values from 0 - 59
	int min = minute();  // Values from 0 - 59
	int hour = hour();    // Values from 0 - 23

	String logname = new String();
	String monthString = new String();

printThisLine = false;
newline = 1;
currentChar = 0;

	 switch (mo) {
            case 1:  monthString = "January";       break;
            case 2:  monthString = "February";      break;
            case 3:  monthString = "March";         break;
            case 4:  monthString = "April";         break;
            case 5:  monthString = "May";           break;
            case 6:  monthString = "June";          break;
            case 7:  monthString = "July";          break;
            case 8:  monthString = "August";        break;
            case 9:  monthString = "September";     break;
            case 10: monthString = "October";       break;
            case 11: monthString = "November";      break;
            case 12: monthString = "December";      break;
            default: monthString = "Invalid month"; break;
        }

	logname = "log-" + monthString + "-" + day + "-" + hour + "-" + min +"-" + sec + ".log";
	println(logname);
	println(Serial.list());
	myPort = new Serial(this, Serial.list()[0], 9600);

	// Create a new file in the sketch directory
	outputLog = createWriter(logname);
	
	
		outputLog.print("checking checking checking\n");
}



void draw()
{
	String tempString = new String(); 


		while (myPort.available() > 0)
		{

			currentChar = myPort.readChar();

			if(currentChar == '\n' )
			{
				//println(printThisLine);
				if(printThisLine == false)
					outputLog.print('\n');
				outputLog.flush();

			}
			if(currentChar == '!')
			{
				outputLog.print('\n');
				printThisLine = false;
			}
			else if(currentChar == '$')
			{
				
				printThisLine = true;
			}
			else 
			{
				if(printThisLine)
					print(currentChar);
				else
					outputLog.print(currentChar);
			}

				

		}
}

void keyPressed() {

	myPort.write(key);

}
