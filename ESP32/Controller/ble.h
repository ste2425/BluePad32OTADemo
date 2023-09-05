#ifndef BLE_H
#define BLE_H
  #include "att_setup.h"

  // define the types for our read and write callbacks
  typedef const char *(*OnReadCallback)();
  typedef void (*OnWriteCallback)(uint8_t *buffer);

  void ble_setup(OnReadCallback onRead, OnWriteCallback onWrite);

#endif