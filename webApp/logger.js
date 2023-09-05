export class Logger {
    /** @type {Element} */
    #log;

    /** @param {string} loggerSelector  */
    constructor(loggerSelector) {
        this.#log = document.querySelector(loggerSelector);
    }

    /** @param {string} message  */
    log(message) {
        this.#log.textContent += `\n${message}`;
    }

    logPretty(obj) {
        this.log(
            JSON.stringify(obj, null, 4)
        );
    }
}