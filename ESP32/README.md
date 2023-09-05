# Bluepad32 BLE Example

This example is an extension of the controller example provided by BluePad32.

It sets up an ATT Server with a single service (`0000FF10-0000-1000-8000-00805F9B34FB`) and a single characteristic (`0000FF11-0000-1000-8000-00805F9B34FB`) configured for `read` and `write`.

## The .gatt file
Services and profiles are configured in a `.gatt` file then compiled into a header file (`att_setup.h`) and included in the application.

In order to change services and characteristics (remove/create and changing UUID's) the `.gatt` file should be modified and the header re-generated.

The python script `compile_gatt.py <gatt file> <header file>` provided by the BTStack library can be used. (view its source [here](https://github.com/bluekitchen/btstack/blob/master/tool/compile_gatt.py))

For example: `compile_gatt.py att_setup.gatt att_setup.h`

## Core 0 and the BTStack main thread

BTstack is the Bluetooth library in use. As BluePad32 deals with the main setup of BTStack and BluePad32 runs on core 0 we also need to run our own BTStack commands on core 0. This is to prevent potential memory corruption issues. Imagine both cores do some BTStack actions at the same time etc.

On top of that according to BTStack documentation [here](https://github.com/bluekitchen/btstack/blob/fa58ca9cd7aaaa9036bb1480b2d5ee43a6985b24/port/esp32/README.md?plain=1#L91) we need to run commands interacting with BTStack in its main thread.

Code being run by BTStack, such as read and write handlers are thread safe however. That means we can interact with BTStack in these handlers without needing to manually run code in BTStacks main thread.

In practice in this example only the BTStack setup code needs to be manually run on core 0 and on BTStacks main thread.
