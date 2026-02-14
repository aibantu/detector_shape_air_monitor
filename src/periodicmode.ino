// /*

// This is example for SHT3X-DIS Digital Humidity & Temperature Sensors Arduino Library
// ClosedCube SHT30D-EASY or ClosedCube SHT31D-PRO breakout modules

// Initial Date: 06-Oct-2015

// Hardware connections for Arduino Uno:
// 	VDD to 3.3V DC
// 	SDA to A4
// 	SCL to A5
// 	GND to common groud

// MIT License

// */

// #include <Wire.h>
// #include <ClosedCube_SHT3XD.h>

// ClosedCube_SHT3XD sht3xd;

// void setup()
// {
// 	Serial.begin(9600);
// 	Serial.println("ClosedCube SHT3XD Library - Periodic Mode Example");
	
// 	sht3xd.begin(0x44);
// 	Serial.print("SHT3XD Serial #");
// 	Serial.println(sht3xd.readSerialNumber());	

// 	if (sht3xd.periodicStart(REPEATABILITY_HIGH, FREQUENCY_10HZ) != NO_ERROR)
// 		Serial.println("[ERROR] Cannot start periodic mode");

// }

// void loop()
// {
// 	printResult("Periodic Mode", sht3xd.periodicFetchData());
// 	delay(250);
// }

// void printResult(String text, SHT3XD result) {
// 	if (result.error == NO_ERROR) {
// 		Serial.print(text);
// 		Serial.print(": T=");
// 		Serial.print(result.t);
// 		Serial.print("C, RH=");
// 		Serial.print(result.rh);
// 		Serial.println("%");
// 	}
// 	else {
// 		Serial.print(text);
// 		Serial.print(": [ERROR] Code #");
// 		Serial.println(result.error);
// 	}
// }