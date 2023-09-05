# Bluepad 32 Webapp Example

This example is a simple web app that uses the Web Bluetooth api's to communicate with an ESP32 running the equivalent Bluepad32 example code.

This app is a plain static site. The HTML and JS files can be served any way you wish.

However it does include a package JSON with some packages predefined to make this easier when running locally. These packages include types so you get correct Bluetooth typing in supported editors (VSCode for one) and the `http-server` library to deal with serving your application for you.

With a terminal change directory to this folder then install the packages with `npm install` (This requires NodeJS), once installed run the app with `npm start`.

This will host the app on port `8080` and automatically open your default browser.

## Browser support

This app uses modern JavaScript features:

* ESModules
* Promises
* Async/Await
* Arrow functions

As such it needs to run on modern browsers.

Also it is dependant on the `Web BLuetooth` API's which are currently only supported by `Chromium` based browsers (Chrome, Edge etc).

*NOTE* due to security concerns around Bluetooth the WebBluetooth API's will be disabled unless the app is hosted on `HTTPS` or access on `localhost` (not `127.0.0.1` etc).

