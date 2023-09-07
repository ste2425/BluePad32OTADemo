import { Logger } from './logger.js';
import { primaryServiceUUID, characteristicUUID, otaCharactertisticUUID } from './constants.js';

/** @type {BluetoothRemoteGATTCharacteristic} */
let characteristic,   
    /** @type {BluetoothRemoteGATTCharacteristic} */
    otaCharacteristic;

/** @type {BluetoothDevice} */
let device;

// OTA variables
    let totalSize = 0,
        remaining = 0,
        amountToWrite = 0,
        currentPosition = 0;

    /** @type {ArrayBuffer} */
    let update;

    const blockSize = 244;
// ------

/**
 * Connects to the ESP32. Then connects to its primary service and finally connects to the characteristic to be used later.
 * There is only a small number of steps needed but the logging makes it look more verbose than it is.
 * 
 * @param {Logger} logger 
 */
export async function btConnect(logger) {
    try {
        logger.log('Requesting Bluetooth Device...');

        device = await navigator.bluetooth.requestDevice(
            {
                filters: [{
                    name: 'BluePad'
                }],
                optionalServices: [primaryServiceUUID]
            }
        );
    
        logger.log('Connected');
        
        logger.logPretty({
            id: device.id,
            name: device.name
        });
    
        logger.log('Connecting to GATT Server');
    
        device.addEventListener('gattserverdisconnected', () => {
            logger.log('GATT Server disconnected');

            characteristic = undefined;
        });
    
        const gattServer = await device.gatt.connect();
    
        logger.log('GATT Server Connected');
    
        logger.log(`Requesting Primary service with UUID: ${primaryServiceUUID}`);
    
        const primaryService = await gattServer.getPrimaryService(primaryServiceUUID);
    
        logger.log('Got Primary Service');
    
        logger.log(`Requesting characteristic with UUID: ${characteristicUUID}`);
    
        characteristic = await primaryService.getCharacteristic(characteristicUUID);
    
        logger.log(`Got Characteristic`);
        
        logProperties(characteristic.properties, logger);

        otaCharacteristic = await primaryService.getCharacteristic(otaCharactertisticUUID);
    
        logger.log(`Got OTA Characteristic`);
        
        logProperties(otaCharacteristic.properties, logger);

    } catch (e) {
        logger.log('Error:');
        logger.logPretty({
            message: e.message,
            stack: e.stack
        });
    }
}

/** 
 * Sends a value to the connected characterisic
 * Values have to be encoded as Array Buffers. If sending a string `TextEncoder` can be used.
 * 
 * @param {ArrayBuffer} value
 * @param {Logger} logger 
 */
export async function sendValue(value, logger) {
    if (!characteristic)
        return;
    
    logger.log(`Sending value: ${value}`);

    await characteristic.writeValueWithoutResponse(value);

    logger.log('Sent');
}

/** 
 * Sends a value to the connected characterisic
 * Values have to be encoded as Array Buffers. If sending a string `TextEncoder` can be used.
 * 
 * @param {Logger} logger 
 */
export async function sendNextBlock(logger, setProgres) {
    if (!otaCharacteristic)
        return;

    if (remaining <= 0) {
        logger.log('Complete');
        //device.gatt?.disconnect();
        return;
    }
    
    if (remaining >= blockSize) {
        amountToWrite = blockSize;
    } else {
        amountToWrite = remaining;
    }

    setProgres(currentPosition);

    //const dataToSend = update.slice(currentPosition, currentPosition + amountToWrite);
    const dataToSend = new DataView(update, currentPosition, amountToWrite);
    
    currentPosition += amountToWrite;
    remaining -= amountToWrite;

    logger.log(`Ammount remaining: ${remaining}`);

    await otaCharacteristic.writeValueWithoutResponse(dataToSend);

    logger.log('Sent');

    //await sendNextBlock(logger, setProgres);
}

/** 
 * Sends a value to the connected characterisic
 * Values have to be encoded as Array Buffers. If sending a string `TextEncoder` can be used.
 * 
 * @param {ArrayBuffer} update
 * @param {Logger} logger 
 */
export async function beginOTA(_update, logger, setProgres) {
    if (!otaCharacteristic)
        return;

    update = _update;
    totalSize = update.byteLength;
    remaining = totalSize;
    amountToWrite = 0;
    currentPosition = 0;

    otaCharacteristic.addEventListener('characteristicvaluechanged', async (e) => {
        await sendNextBlock(logger, setProgres);
    });

    await otaCharacteristic.startNotifications();

    await sendNextBlock(logger, setProgres);

    //await otaCharacteristic.stopNotifications();
}

/** 
 * Reads a value from the connected characteristic
 * 
 * @param {Logger} logger 
 */
export async function readValue(logger) {
    if (!characteristic)
        return;

    logger.log('Reading value...');

    const value = await characteristic.readValue();

    const decoder = new TextDecoder('utf-8');
    const decodedValue = decoder.decode(value);

    logger.log(`Got value: ${decodedValue}`);
}

/** 
 * Utility method to log a characteristics properties.
 * `JSON.stringify` on its own will not work as the properties are non enumerable so we need to manually build an object to log.
 * 
 * @param {BluetoothCharacteristicProperties} props  
 * @param {Logger} logger 
*/
function logProperties(props, logger) {
    logger.logPretty({
        broadcast: props.broadcast,
        read: props.read,
        writeWithoutResponse: props.writeWithoutResponse,
        write: props.write,
        notify: props.notify,
        indicate: props.indicate,
        authenticatedSignedWrites: props.authenticatedSignedWrites,
        reliableWrite: props.reliableWrite,
        writableAuxiliaries: props.writableAuxiliaries
    });
}