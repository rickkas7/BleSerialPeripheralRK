
#include "BleSerialPeripheralRK.h"

static const BleUuid serviceUuid("6E400001-B5A3-F393-E0A9-E50E24DCCA9E");
static const BleUuid rxUuid("6E400002-B5A3-F393-E0A9-E50E24DCCA9E");
static const BleUuid txUuid("6E400003-B5A3-F393-E0A9-E50E24DCCA9E");

// #define ENABLE_LOG_DEBUG
#ifdef ENABLE_LOG_DEBUG
# define LOG_DEBUG(x) Log.info x;
#else
# define LOG_DEBUG(x)
#endif

BleSerialPeripheralBase::BleSerialPeripheralBase(uint8_t *txBuf, size_t txBufSize, uint8_t *rxBuf, size_t rxBufSize) :
			txBuf(txBuf), txBufSize(txBufSize),
			txCharacteristic("tx", BleCharacteristicProperty::NOTIFY, txUuid, serviceUuid),
			rxCharacteristic("rx", BleCharacteristicProperty::WRITE_WO_RSP, rxUuid, serviceUuid, onDataReceivedStatic, this),
			rxRing(rxBuf, rxBufSize) {

}


BleSerialPeripheralBase::~BleSerialPeripheralBase() {

}

void BleSerialPeripheralBase::setup() {
	BLE.addCharacteristic(txCharacteristic);
	BLE.addCharacteristic(rxCharacteristic);

	os_mutex_create(&mutex);

	LOG_DEBUG(("setup complete"));
}

void BleSerialPeripheralBase::loop() {
    if (txCur > 0 && BLE.connected()) {
    	LOG_DEBUG(("txCur=%u", txCur));

    	BleSerialPeripheralLock lock(this);

    	size_t count = txCur;
    	if (count > txMaxWrite) {
    		count = txMaxWrite;
    	}
        txCharacteristic.setValue(txBuf, count);
    	LOG_DEBUG(("sent count=%u", count));

    	if (count < txCur) {
    		// If transmitting a partial buffer, move the data to the beginning of the buffer.
    		// This is an unusual case so the inefficiency of this is not that important.
    		memmove(txBuf, &txBuf[count], txCur - count);
    	}

        txCur -= count;
    }
}

void BleSerialPeripheralBase::advertise() {
	BleAdvertisingData data;
	data.appendServiceUUID(serviceUuid);
	BLE.advertise(&data);
	LOG_DEBUG(("advertising"));
}

BleUuid BleSerialPeripheralBase::getServiceUuid() const {
	return serviceUuid;
}

void BleSerialPeripheralBase::lock() {
	os_mutex_lock(mutex);
}

void BleSerialPeripheralBase::unlock() {
	os_mutex_unlock(mutex);
}

int BleSerialPeripheralBase::available() {
	return rxRing.availableForRead();
}
int BleSerialPeripheralBase::read() {
	int result = -1;

	uint8_t *p = rxRing.preRead();
	if (p) {
		result = (int) *p;
		rxRing.postRead();
	}
	return result;
}
int BleSerialPeripheralBase::peek() {
	int result = -1;

	uint8_t *p = rxRing.preRead();
	if (p) {
		result = (int) *p;

		// For peek, don't call postRead() so the data will stay in the buffer
	}
	return result;
}

void BleSerialPeripheralBase::flush() {

}

size_t BleSerialPeripheralBase::write(uint8_t data) {
	BleSerialPeripheralLock lock(this);

	if (txCur < txBufSize) {
		txBuf[txCur++] = data;
		return 1;
	}
	else {
		txLost++;
		return -1;
	}
}



void BleSerialPeripheralBase::onDataReceived(const uint8_t* data, size_t len, const BlePeerDevice& peer) {
	// Log.trace("Received data from: %02X:%02X:%02X:%02X:%02X:%02X:", peer.address()[0], peer.address()[1], peer.address()[2], peer.address()[3], peer.address()[4], peer.address()[5]);

	LOG_DEBUG(("dataReceived %u", len));

	// Copy data into the buffer. Discards data that will not fit!
	for(size_t ii = 0; ii < len; ii++) {
		if (!rxRing.write(&data[ii])) {
			rxLost++;
		}
	}
}

// [static]
void BleSerialPeripheralBase::onDataReceivedStatic(const uint8_t* data, size_t len, const BlePeerDevice& peer, void* context) {
	BleSerialPeripheralBase *This = (BleSerialPeripheralBase *)context;

	This->onDataReceived(data, len, peer);
}



/*
 * #include "Particle.h"

// This example does not require the cloud so you can run it in manual mode or
// normal cloud-connected mode
SYSTEM_MODE(MANUAL);

const size_t UART_TX_BUF_SIZE = 20;

void onDataReceived(const uint8_t* data, size_t len, const BlePeerDevice& peer, void* context);

// These UUIDs were defined by Nordic Semiconductor and are now the defacto standard for
// UART-like services over BLE. Many apps support the UUIDs now, like the Adafruit Bluefruit app.
const BleUuid serviceUuid("6E400001-B5A3-F393-E0A9-E50E24DCCA9E");
const BleUuid rxUuid("6E400002-B5A3-F393-E0A9-E50E24DCCA9E");
const BleUuid txUuid("6E400003-B5A3-F393-E0A9-E50E24DCCA9E");

BleCharacteristic txCharacteristic("tx", BleCharacteristicProperty::NOTIFY, txUuid, serviceUuid);
BleCharacteristic rxCharacteristic("rx", BleCharacteristicProperty::WRITE_WO_RSP, rxUuid, serviceUuid, onDataReceived, NULL);

void onDataReceived(const uint8_t* data, size_t len, const BlePeerDevice& peer, void* context) {
    // Log.trace("Received data from: %02X:%02X:%02X:%02X:%02X:%02X:", peer.address()[0], peer.address()[1], peer.address()[2], peer.address()[3], peer.address()[4], peer.address()[5]);

    for (size_t ii = 0; ii < len; ii++) {
        Serial.write(data[ii]);
    }
}

void setup() {
    Serial.begin();

    BLE.addCharacteristic(txCharacteristic);
    BLE.addCharacteristic(rxCharacteristic);

    BleAdvertisingData data;
    data.appendServiceUUID(serviceUuid);
    BLE.advertise(&data);
}

void loop() {
    if (BLE.connected()) {
    	uint8_t txBuf[UART_TX_BUF_SIZE];
    	size_t txLen = 0;

    	while(Serial.available() && txLen < UART_TX_BUF_SIZE) {
            txBuf[txLen++] = Serial.read();
            Serial.write(txBuf[txLen - 1]);
        }
        if (txLen > 0) {
            txCharacteristic.setValue(txBuf, txLen);
        }
    }
}
 *
 */
