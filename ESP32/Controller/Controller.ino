// Copyright 2021 - 2023, Ricardo Quesada
// SPDX-License-Identifier: Apache 2.0 or LGPL-2.1-or-later

/*
 * This example shows how to use the Controller API.
 *
 * Supported on boards with NINA W10x. In particular these boards:
 *  - Arduino MKR WiFi 1010,
 *  - UNO WiFi Rev.2,
 *  - Nano RP2040 Connect,
 *  - Nano 33 IoT,
 *  - Arduino Arduino MKR Vidor 4000
 */
#include <Bluepad32.h>
#include "ble.h"

ControllerPtr myControllers[BP32_MAX_CONTROLLERS];

// Arduino setup function. Runs in CPU 1
void setup() {
  // Initialize serial
  Serial.begin(115200);
  while (!Serial) {
    // wait for serial port to connect.
    // You don't have to do this in your game. This is only for debugging
    // purposes, so that you can see the output in the serial console.
    ;
  }

  String fv = BP32.firmwareVersion();
  Serial.print("Firmware version installed: ");
  Serial.println(fv);

  // To get the BD Address (MAC address) call:
  const uint8_t* addr = BP32.localBdAddress();
  Serial.print("BD Address: ");
  for (int i = 0; i < 6; i++) {
    Serial.print(addr[i], HEX);
    if (i < 5)
      Serial.print(":");
    else
      Serial.println();
  }

  // This call is mandatory. It setups Bluepad32 and creates the callbacks.
  BP32.setup(&onConnectedController, &onDisconnectedController);

  // "forgetBluetoothKeys()" should be called when the user performs
  // a "device factory reset", or similar.
  // Calling "forgetBluetoothKeys" in setup() just as an example.
  // Forgetting Bluetooth keys prevents "paired" gamepads to reconnect.
  // But might also fix some connection / re-connection issues.
  BP32.forgetBluetoothKeys();

  // Setups up our BLE server
  // We provide it with two callbacks which will be executed when a connected device
  // wishes to read data from the ESP or write data to the ESP
  // BLE code runs on core 0 meaning these callbacks will also be executed on core 0
  // The use of xTaskCreatePinnedToCore can be used to run code on a specific core
  ble_setup(BTOnRead, BTOnWrite);
}

// Callback executed when a connected BT device wishes to read data
// The value returned is what will be sent. This demo sends a string but any data type can be sent
const char * BTOnRead() {
  printf("\nONREAD\n");
  return "Hello, World!";
}

// Callback executed when a connected BT device wishes to write data
// The callback recieves the buffer being written. Within the ble.ino handler that executes this
// The size of the buffer is available
void BTOnWrite(uint8_t *buffer) {
  printf("Write request, value: ");
  printf("%d\n", buffer);
  printf("On core: %d\n", xPortGetCoreID());

  ControllerPtr myController = myControllers[0];
  
  // If the value recieved from the connected device is a string of `r`, `b` or `g` set the colour of the
  // connected controller accordingly
  switch (*buffer) {
    case 0x72:
      printf("RED");

      if (myController && myController->isConnected()) {
        myController->setColorLED(255, 0, 0);
      }
    break;
    case 0x67:
      printf("GREEN");

      if (myController && myController->isConnected()) {
        myController->setColorLED(0, 255, 0);
      }
    break;
    case 0x62:
      printf("Blue");

      if (myController && myController->isConnected()) {
        myController->setColorLED(0, 0, 255);
      }
    break;
  }
}

// This callback gets called any time a new gamepad is connected.
// Up to 4 gamepads can be connected at the same time.
void onConnectedController(ControllerPtr ctl) {
  bool foundEmptySlot = false;
  for (int i = 0; i < BP32_MAX_GAMEPADS; i++) {
    if (myControllers[i] == nullptr) {
      Serial.print("CALLBACK: Controller is connected, index=");
      Serial.println(i);
      myControllers[i] = ctl;
      foundEmptySlot = true;

      // Optional, once the gamepad is connected, request further info about the
      // gamepad.
      ControllerProperties properties = ctl->getProperties();
      char buf[80];
      sprintf(buf,
              "BTAddr: %02x:%02x:%02x:%02x:%02x:%02x, VID/PID: %04x:%04x, "
              "flags: 0x%02x",
              properties.btaddr[0], properties.btaddr[1], properties.btaddr[2],
              properties.btaddr[3], properties.btaddr[4], properties.btaddr[5],
              properties.vendor_id, properties.product_id, properties.flags);
      Serial.println(buf);
      break;
    }
  }
  if (!foundEmptySlot) {
    Serial.println(
        "CALLBACK: Controller connected, but could not found empty slot");
  }
}

void onDisconnectedController(ControllerPtr ctl) {
  bool foundGamepad = false;

  for (int i = 0; i < BP32_MAX_GAMEPADS; i++) {
    if (myControllers[i] == ctl) {
      Serial.print("CALLBACK: Controller is disconnected from index=");
      Serial.println(i);
      myControllers[i] = nullptr;
      foundGamepad = true;
      break;
    }
  }

  if (!foundGamepad) {
    Serial.println(
        "CALLBACK: Controller disconnected, but not found in myControllers");
  }
}

void processGamepad(ControllerPtr gamepad) {
  // There are different ways to query whether a button is pressed.
  // By query each button individually:
  //  a(), b(), x(), y(), l1(), etc...

  if (gamepad->a()) {
    static int colorIdx = 0;
    // Some gamepads like DS4 and DualSense support changing the color LED.
    // It is possible to change it by calling:
    switch (colorIdx % 3) {
      case 0:
        // Red
        gamepad->setColorLED(255, 0, 0);
        break;
      case 1:
        // Green
        gamepad->setColorLED(0, 255, 0);
        break;
      case 2:
        // Blue
        gamepad->setColorLED(0, 0, 255);
        break;
    }
    colorIdx++;
  }

  if (gamepad->b()) {
    // Turn on the 4 LED. Each bit represents one LED.
    static int led = 0;
    led++;
    // Some gamepads like the DS3, DualSense, Nintendo Wii, Nintendo Switch
    // support changing the "Player LEDs": those 4 LEDs that usually indicate
    // the "gamepad seat".
    // It is possible to change them by calling:
    gamepad->setPlayerLEDs(led & 0x0f);
  }

  if (gamepad->x()) {
    // Duration: 255 is ~2 seconds
    // force: intensity
    // Some gamepads like DS3, DS4, DualSense, Switch, Xbox One S support
    // rumble.
    // It is possible to set it by calling:
    gamepad->setRumble(0xc0 /* force */, 0xc0 /* duration */);
  }

  // Another way to query the buttons, is by calling buttons(), or
  // miscButtons() which return a bitmask.
  // Some gamepads also have DPAD, axis and more.
  char buf[256];
  snprintf(buf, sizeof(buf) - 1,
           "idx=%d, dpad: 0x%02x, buttons: 0x%04x, "
           "axis L: %4li, %4li, axis R: %4li, %4li, "
           "brake: %4ld, throttle: %4li, misc: 0x%02x, "
           "gyro x:%6d y:%6d z:%6d, accel x:%6d y:%6d z:%6d, "
           "battery: %d",
           gamepad->index(),        // Gamepad Index
           gamepad->dpad(),         // DPAD
           gamepad->buttons(),      // bitmask of pressed buttons
           gamepad->axisX(),        // (-511 - 512) left X Axis
           gamepad->axisY(),        // (-511 - 512) left Y axis
           gamepad->axisRX(),       // (-511 - 512) right X axis
           gamepad->axisRY(),       // (-511 - 512) right Y axis
           gamepad->brake(),        // (0 - 1023): brake button
           gamepad->throttle(),     // (0 - 1023): throttle (AKA gas) button
           gamepad->miscButtons(),  // bitmak of pressed "misc" buttons
           gamepad->gyroX(),      // Gyro X
           gamepad->gyroY(),      // Gyro Y
           gamepad->gyroZ(),      // Gyro Z
           gamepad->accelX(),     // Accelerometer X
           gamepad->accelY(),     // Accelerometer Y
           gamepad->accelZ(),     // Accelerometer Z
           gamepad->battery()       // 0=Unknown, 1=empty, 255=full
  );
  Serial.println(buf);

  // You can query the axis and other properties as well. See
  // Controller.h For all the available functions.
}

// Arduino loop function. Runs in CPU 1
void loop() {
  // This call fetches all the controller info from the NINA (ESP32) module.
  // Just call this function in your main loop.
  // The controllers pointer (the ones received in the callbacks) gets updated
  // automatically.
  BP32.update();

  // It is safe to always do this before using the controller API.
  // This guarantees that the controller is valid and connected.
  for (int i = 0; i < BP32_MAX_CONTROLLERS; i++) {
    ControllerPtr myController = myControllers[i];

    if (myController && myController->isConnected()) {
      if (myController->isGamepad())
        processGamepad(myController);
    }
  }
  delay(150);
}
