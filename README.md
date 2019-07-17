# BleSerialPeripheralRK

*Library to simplify using BLE UART peripheral mode on Gen 3 devices*

## Introduction

Particle Gen 3 devices (Argon, Boron, Xenon) running Device OS 1.3.0-rc.1 and later have support for BLE (Bluetooth Low Entergy) in central and peripheral roles. 

Nordic Semiconductor created a UART peripheral protocol to allow central devices (like mobile phones) to connect to a BLE device and read UART-like data streams. This is supported not only by the nRF Toolbox app, but also some other apps like the Adafruit Bluefruit app.

There is [a code example in the docs](https://docs.particle.io/tutorials/device-os/bluetooth-le/#uart-peripheral), however this class encapsulates the BLE stuff and makes a Serial-like interface to it. Among the benefits:

- Reading is easy using standard functions like read(), readUntil(), readString(), etc. like you can from Serial, Serial1, etc..
- Writing is easy and buffered, allowing not only write() to write a byte, but also all of the variations of print(), println(), printf(), printlnf(), etc. that are available when using Serial, etc..
- All of the BLE stuff is encapsulated so you don't have to worry about it.

Documentation can be found at: [https://rickkas7.github.io/BleSerialPeripheralRK/index.html](https://rickkas7.github.io/BleSerialPeripheralRK/index.html)

Github repository: [https://github.com/rickkas7/BleSerialPeripheralRK](https://github.com/rickkas7/BleSerialPeripheralRK)

License: MIT 


## Example

There is one example in 1-simple-BleSerialPeripheralRK:

```
#include "BleSerialPeripheralRK.h"

SerialLogHandler logHandler;

SYSTEM_THREAD(ENABLED);

// First parameter is the transmit buffer size, second parameter is the receive buffer size
BleSerialPeripheralStatic<32, 256> bleSerial;

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
```

Among the important things:

You normally instantiate a BleSerialPeripheralStatic object as a global variable. The first number in the <> is the transmit buffer size and the second is the receive buffer size.

```
// First parameter is the transmit buffer size, second parameter is the receive buffer size
BleSerialPeripheralStatic<256, 256> bleSerial;
```

Because the data is buffered and only sent from loop() the transmit buffer must be larger than the amount of data you intend to sent at once, or the maximum data that will accumulate between calls to loop.

Likewise, since data is read from loop but received asynchronously by BLE, you must have a receive buffer that is large enough to hold any data between times you will be processing it from your loop function.

If there is a data overflow situation, the data is discarded.

Be sure to call this from your setup() function. 

```
bleSerial.setup();
```

If you are only using BLE UART you can call:

```
bleSerial.advertise();
```

If you are advertising multiple services, instead call `bleSerial.getServiceUuid()` to get the UART serial UUID and add it with your own services:

```
BleAdvertisingData data;
data.appendServiceUUID(bleSerial.getServiceUuid());
// append your own service UUIDs here
BLE.advertise(&data);
```

Be sure to call this from loop, as often as possible.

```
bleSerial.loop();
```

In this example we used `bleSerial.readString()` but there are many method of the Stream class to read. Beware of blocking, however. If you are waiting to read a string, you won't be calling `bleSerial.loop()` and data won't be transmitted during that time.

Finally, you can print to BLE serial using all of the standard Stream methods. The example uses `bleSerial.printlnf()` to print a line using `sprintf` formatting.

