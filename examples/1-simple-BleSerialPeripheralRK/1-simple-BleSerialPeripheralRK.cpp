#include "BleSerialPeripheralRK.h"

SerialLogHandler logHandler;

SYSTEM_THREAD(ENABLED);

// First parameter is the transmit buffer size, second parameter is the receive buffer size
BleSerialPeripheralStatic<256, 256> bleSerial;

const unsigned long TRANSMIT_PERIOD_MS = 2000;
unsigned long lastTransmit = 0;
int counter = 0;


void setup() {
	Serial.begin();

	// This must be called from setup()!
	bleSerial.setup();

	// If you don't have any other services to advertise, just call advertise().
	// Otherwise, call getServiceUuid() to get the serial service UUID and add that to your
	// custom advertising data payload and call BLE.advertise() yourself with all of your necessary
	// services added.
	bleSerial.advertise();
}

void loop() {
	// This must be called from loop() on every call to loop.
	bleSerial.loop();

	// Print out anything we receive
	if(bleSerial.available()) {
		String s = bleSerial.readString();

		Log.info("received: %s", s.c_str());
	}

	if (millis() - lastTransmit >= TRANSMIT_PERIOD_MS) {
		lastTransmit = millis();

		// Every two seconds, send something to the other side
		bleSerial.printlnf("testing %d", ++counter);
		Log.info("counter=%d", counter);
	}
}

