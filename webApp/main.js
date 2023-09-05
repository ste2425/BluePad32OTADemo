import { Logger } from './logger.js';
import { btConnect, readValue, sendValue } from './bt.js';

/** @type {Logger} */
let logger;

/**
 * Main entry point to the application, queries the DOM and registers even handlers to perform Bluetooth actions.
 */
document.addEventListener('DOMContentLoaded', () => {
    logger = new Logger('#logger');

    document.getElementById('connectBTN').addEventListener('click', () => btConnect(logger));

    /** @type {HTMLInputElement} */
    const valueToSend = document.querySelector('#valueToSend');

    document.getElementById('send').addEventListener('click', () => {
      const encoder = new TextEncoder();
       sendValue(encoder.encode(valueToSend.value), logger);
    });
    
    document.getElementById('readValue').addEventListener('click', () => readValue(logger));
});
