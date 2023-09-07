import { Logger } from './logger.js';
import { btConnect, readValue, sendValue, beginOTA } from './bt.js';

/** @type {Logger} */
let logger;

/** @type {HTMLProgressElement} */
let progress;

/**
 * Main entry point to the application, queries the DOM and registers even handlers to perform Bluetooth actions.
 */
document.addEventListener('DOMContentLoaded', () => {
    logger = new Logger('#logger');

    document.getElementById('connectBTN').addEventListener('click', () => btConnect(logger));

    /** @type {HTMLInputElement} */
    const valueToSend = document.querySelector('#valueToSend');

    /** @type {HTMLDialogElement} */
    const otaModal = document.getElementById('dialogModal');

    /** @type {HTMLButtonElement} */
    const otaUpgradeBtn = document.getElementById('otaUpgrade');

    /** @type {HTMLInputElement} */
    let fileInput = document.getElementById('otaUpdate');

    document.getElementById('send').addEventListener('click', () => {
      const encoder = new TextEncoder();
      sendValue(encoder.encode(valueToSend.value), logger);
    });
    
    document.getElementById('readValue').addEventListener('click', () => readValue(logger));

    fileInput.addEventListener('change', ({ target }) => {
      otaInputChange(target);
    });

    document.getElementById('otaDialogBtn').addEventListener('click', () => otaModal.showModal());    
    
    document.getElementById('otaClose').addEventListener('click', () => {
      otaModal.close();
      otaUpgradeBtn.setAttribute('disabled', 'disabled');
      fileInput.value = null;
    });

    otaUpgradeBtn.addEventListener('click', () => {
      otaInputChange(fileInput);
    });

    progress = document.getElementById('otaProgress');
});

/**
 * 
 * @param {HTMLInputElement} input 
 */
function otaInputChange(input) {
  const file = input.files[0];

  if (!file)
    return;

  var fr = new FileReader();
  fr.onload = function () {
      var data = fr.result;
      progress.max = data.byteLength;

      beginOTA(data, logger, (completed) => {
        progress.value = completed;
      });
  };
  fr.readAsArrayBuffer(file);
}