// ESP32-S3 + SHT3X-D (I2C) example
// SDA -> GPIO13, SCL -> GPIO14
// Reads temperature/humidity every 3 seconds and prints to Serial.

#include <Arduino.h>
#include <Wire.h>

#include <ClosedCube_SHT3XD.h>

static ClosedCube_SHT3XD sht3xd;

static void println2(const __FlashStringHelper* s)
{
	Serial.println(s);
}

static void print2(const __FlashStringHelper* s)
{
	Serial.print(s);
}

static void println2u(uint32_t v)
{
	Serial.println(v);
}

static void print2f(float v, int digits)
{
	Serial.print(v, digits);
}

static uint8_t i2cPing(uint8_t address)
{
	Wire.beginTransmission(address);
	return Wire.endTransmission(true);
}

static void i2cScanOnce()
{
	println2(F("I2C scan:"));
	uint8_t found = 0;
	for (uint8_t addr = 1; addr < 127; addr++)
	{
		uint8_t rc = i2cPing(addr);
		if (rc == 0)
		{
			print2(F(" - found 0x"));
			Serial.println(addr, HEX);
			found++;
		}
	}
	if (found == 0)
	{
		println2(F(" - none (check wiring/pull-ups/pins)"));
	}
}

static void printResult(const __FlashStringHelper* label, const SHT3XD& result)
{
	if (result.error == NO_ERROR)
	{
		print2(label);
		print2(F(": T="));
		print2f(result.t, 2);
		print2(F("C, RH="));
		print2f(result.rh, 2);
		println2(F("%"));
	}
	else
	{
		print2(label);
		print2(F(": [ERROR] Code #"));
		Serial.println((int)result.error);
	}
}

static bool initSht3xd(uint8_t address, uint8_t sdaPin, uint8_t sclPin, uint32_t i2cFreqHz)
{
	sht3xd.begin(address);
	// begin() internally calls Wire.begin() without pins; re-apply our pin mapping.
	Wire.begin(sdaPin, sclPin);
	Wire.setClock(i2cFreqHz);
	#if defined(ESP32)
		Wire.setTimeOut(50);
	#endif

	print2(F("Trying SHT3XD addr 0x"));
	Serial.print(address, HEX);
	println2(F("..."));

	uint8_t pingRc = i2cPing(address);
	if (pingRc != 0)
	{
		print2(F("[ERROR] I2C NACK/err on address, rc="));
		Serial.println(pingRc);
		return false;
	}

	// Try a soft reset to recover from any previous bus/sensor state.
	(void)sht3xd.softReset();
	delay(10);

	print2(F("SHT3XD Serial #"));
	uint32_t sn = sht3xd.readSerialNumber();
	println2u(sn);
	if (sn == 0)
	{
		println2(F("[WARN] Serial number is 0 (read failed or sensor not responding fully)"));
	}

	SHT3XD_ErrorCode rc = sht3xd.periodicStart(REPEATABILITY_HIGH, FREQUENCY_1HZ);
	if (rc != NO_ERROR)
	{
		print2(F("[ERROR] periodicStart failed, code #"));
		Serial.println((int)rc);
		return false;
	}

	return true;
}

// NOTE: This file is kept as an I2C/SHT3X-D reference implementation.
// The project entry point is src/main.cpp; do NOT define Arduino setup()/loop() here.

