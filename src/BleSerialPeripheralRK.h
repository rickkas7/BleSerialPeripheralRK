#ifndef __BLESERIALPERIPHERAL_H
#define __BLESERIALPERIPHERAL_H

#include "Particle.h"
#include "RingBuffer.h"

/**
 * @brief Main class for BLE UART funtionality
 *
 * Normally you'll instantiate a BleSerialPeripheralStatic as a global variable instead
 * of using this class.
 *
 * You can only have one instance of this object per device!
 *
 * Note: This method subclasses Stream.
 *
 * Documentation can be found at:
 * https://rickkas7.github.io/BleSerialPeripheralRK
 *
 * Github repository:
 * https://github.com/rickkas7/BleSerialPeripheralRK
 *
 * License: MIT
 */
class BleSerialPeripheralBase : public Stream {
public:
	/**
	 * @brief Construct an object with passed-in buffers
	 *
	 * @param txBuf Pointer to buffer for tx (this device to the other device)
	 * @param txBufSize Size of the buffer in bytes
	 * @param rxBuf Pointer to the buffer for rx (the other device to this device)
	 * @param rxBufSize Size of the buffer in bytes
	 *
	 * Normally you'd use BleSerialPeripheralStatic as a global variable, but in unusual cases you can
	 * construct the object with this constructor and pass in the buffers to use.
	 */
	BleSerialPeripheralBase(uint8_t *txBuf, size_t txBufSize, uint8_t *rxBuf, size_t rxBufSize);

	/**
	 * @brief Destructor.
	 *
	 * You normally instantiate this object as a global variable so you won't be deleting it.
	 * The buffers you passed in are not freed!
	 */
	virtual ~BleSerialPeripheralBase();

	/**
	 * @brief You must call this from setup() to initialize the object
	 */
	void setup();

	/**
	 * @brief You must call this from loop() to process outgoing data
	 */
	void loop();

	/**
	 * @brief Call this to advertise the BLE UART service
	 *
	 * If you have multiple services you want to advertise, instead of calling this method
	 * use getServiceUuid() to get the UUID of the UART service and add it to your advertising
	 * data.
	 */
	void advertise();

	/**
	 * @brief Get the service UUID for the UART service
	 *
	 * This is used if you advertise multiple services.
	 */
	BleUuid getServiceUuid() const;

	/**
	 * @brief Obtain a mutex lock on the TX buffer.
	 *
	 * This is used internally, you should not normally need to call this.
	 */
	void lock();

	/**
	 * @brief Release the mutex lock on the TX buffer.
	 *
	 * This is used internally, you should not normally need to call this.
	 */
	void unlock();

	/**
	 * @brief Override for Stream class, returns the number of bytes that can be read right now.
	 */
    virtual int available();

    /**
     * @brief Override for the Stream class, reads a byte of data from the input ring buffer.
     */
    virtual int read();

    /**
     * @brief Override for the Stream class, reads a byte of data, but leaves it in the buffer so
     * read() will get the same byte of data.
     */
    virtual int peek();

    /**
     * @brief Override for the Stream class. Currently does nothing.
     */
    virtual void flush();

	/**
	 * @brief Virtual override from the Print class. Writes a byte of data.
	 *
	 * @param data The byte to write
	 * @returns 1 if successful or -1 on failure
	 *
	 * Note that writes are accumulated in the txBuf, then sent out in bunches from the loop
	 * thread. If you overflow the txBuf, then the data is discarded and -1 is returned. The
	 * txLost counter is incremented as well.
	 */
    virtual size_t write(uint8_t data);

    /**
     * Get the number of receive bytes lost because the receive buffer was full.
     *
     * Data is received asynchronously by BLE. If you do not empty the data from loop fast enough
     * or use too small of a buffer, the data will be lost. The rxLost counter is the number of bytes
     * that have been lost.
     */
    inline size_t getRxLost() const { return rxLost; };

    /**
     * @brief Clears the rxLost counter
     */
    inline void clearRxLost() { rxLost = 0; };

    /**
     * Get the number of transmit bytes lost because the transmit buffer was full.
     *
     * Data can be written from other threads and is buffered in the txBuf. During loop(), the data
     * is sent (in chunks up to txMaxWrite bytes). If you write more data than can fit in the txBuf,
     * then the data is lost and the txLost counter is incremented.
     */
    inline size_t getTxLost() const { return txLost; };

    /**
     * @brief Clears the txLost counter
     */
    inline void clearTxLost() { txLost = 0; };

    /**
     * @brief Get the maximum chunk size to send. The default is 236 bytes.
     */
    inline size_t getTxMaxWrite() const { return txMaxWrite; };

    /**
     * @brief Set the maximum chunk size to send.
     *
     * @param value The value in bytes to set.
     *
     * It set larger than 244, any data larger than that will be silently discarded. If your BLE central
     * implementation does not want that much data at once, set it lower.
     *
     * Note that during loop all of the outstanding data up to txMaxWrite bytes will be sent.
     * If there are fewer than txMaxWrite byte then the smaller amount of data will be sent;
     * it does not wait around for more data.
     *
     * Even if you write multiple small writes using write() or print() methods they will be
     * bunched up into more efficient larger writes.
     */
    inline void setTxMaxWrite(size_t value) { txMaxWrite = value; };

private:
    /**
     * @brief Called when data is received by BLE and stores it in the rxRing
     */
	void onDataReceived(const uint8_t* data, size_t len, const BlePeerDevice& peer);

	/**
	 * @brief Static callback function passed to BLE. Note: context must be "this"!
	 */
	static void onDataReceivedStatic(const uint8_t* data, size_t len, const BlePeerDevice& peer, void* context);

	uint8_t *txBuf;
	size_t txBufSize;

	BleCharacteristic txCharacteristic;
	BleCharacteristic rxCharacteristic;

	os_mutex_t mutex;
	RingBuffer<uint8_t> rxRing;

	size_t rxLost = 0;
	size_t txLost = 0;
	size_t txCur = 0;

	size_t txMaxWrite = 236;
};

/**
 * @brief BleSerialPeripheral with static buffers. This is the object you will normally create, as a global variable.
 *
 * TX_BUF_SIZE is the size of the txBuf in bytes. Since data is bunched up and sent out from loop this must be larger
 * than the largest chunk of data you ever intend to send.
 *
 * RX_BUF_SIZE is the size of the rxBuf in bytes. Since data is received asynchronously, this must be larger than
 * the most data you will receive and read in loop().
 *
 * You can only have one BleSerialPeripheralStatic or BleSerialPeripheralBase object per device!
 */
template <size_t TX_BUF_SIZE, size_t RX_BUF_SIZE>
class BleSerialPeripheralStatic : public BleSerialPeripheralBase {
public:
	explicit BleSerialPeripheralStatic() : BleSerialPeripheralBase(txBufStatic, TX_BUF_SIZE, rxBufStatic, RX_BUF_SIZE) {};

private:
	uint8_t txBufStatic[TX_BUF_SIZE]; //!< static transmit buffer
	uint8_t rxBufStatic[RX_BUF_SIZE]; //!< static receive buffer
};

/**
 * @brief Class to lock and unlock the tx mutex automatically. Allocate this object on the stack so when
 * it goes out of scope the mutex will always be released.
 */
class BleSerialPeripheralLock {
public:
	inline BleSerialPeripheralLock(BleSerialPeripheralBase *parent) : parent(parent) {
		parent->lock();
	}

	inline ~BleSerialPeripheralLock() {
		parent->unlock();
	}

protected:
	BleSerialPeripheralBase *parent;
};

#endif /* __BLESERIALPERIPHERAL_H */
