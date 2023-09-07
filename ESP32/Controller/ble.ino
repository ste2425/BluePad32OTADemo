/*
    This file manages BLE related tasks.

    Setting up the ATT Server, listening to read and write events and executing the provided callbacks for these actions.

    All code interacting with BTStack needs to run on core 0
*/
#include "ble.h"

// BLE Connection related handlers
TaskHandle_t ble_setup_core_0_handler; // Run on Core 0
static btstack_context_callback_registration_t btstack_main_thread_connection_handler; // Run BTStack main

// Notification
static int  le_notification_enabled;
static hci_con_handle_t con_handle;
static btstack_packet_callback_registration_t hci_event_callback_registration;

bool updateFlag = false;
esp_ota_handle_t otaHandler = 0;

static void packet_handler (uint8_t packet_type, uint16_t channel, uint8_t *packet, uint16_t size);
static uint16_t att_read_callback(hci_con_handle_t con_handle, uint16_t att_handle, uint16_t offset, uint8_t * buffer, uint16_t buffer_size);
static int att_write_callback(hci_con_handle_t connection_handle, uint16_t att_handle, uint16_t transaction_mode, uint16_t offset, uint8_t *buffer, uint16_t buffer_size);

// Setup advertisment data
const uint8_t adv_data[] = {
    // Flags general discoverable, BR/EDR not supported
    0x02, 0x01, 0x06, 
    // Name
    0x08, 0x09, 'B', 'l', 'u', 'e', 'P', 'a', 'd', 
};
const uint8_t adv_data_len = sizeof(adv_data);

OnReadCallback onReadCallback = nullptr;
OnWriteCallback onWriteCallback = nullptr;

void ble_setup(OnReadCallback onRead, OnWriteCallback onWrite) {
  onReadCallback = onRead;
  onWriteCallback = onWrite;

  // Run the BLE setup code on core 0
  xTaskCreatePinnedToCore(
      ble_setup_core0_task, /* Function to implement the task */
      "Task1", /* Name of the task */
      10000,  /* Stack size in words */
      NULL,  /* Task input parameter */
      0,  /* Priority of the task */
      &ble_setup_core_0_handler,  /* Task handle. */
      0 /* Core where the task should run */
    );
}

// When running BTStack code we need to run in core 0 and in the BTStack main thread
// To do this we first run a task on core0 then from that task use btstack_run_loop_execute_on_main_thread
// To run code on BTStacks main thread.
void ble_setup_core0_task(void * parameter) {
  btstack_main_thread_connection_handler.callback = &ble_setup_btstackmain_task;
  btstack_run_loop_execute_on_main_thread(&btstack_main_thread_connection_handler);
  
  // Tasks being run on another core cannot return a value
  // The task either needs to be deleted when it has done its job
  // Or run forever with an infinite loop
  vTaskDelete(ble_setup_core_0_handler);
}

void ble_setup_btstackmain_task(void * parameter) {
  printf("Setting up ATT Server");
  printf("On core: %d\n", xPortGetCoreID());

  // The profile_data variable contains all our configured services and characterics UUID data it is provided by att_setup.h
  // and auto generated from a .gatt file. See the readme for information on this processes.
  att_server_init(profile_data, att_read_callback, att_write_callback);    
  
  // setup advertisements
  uint16_t adv_int_min = 0x0030;
  uint16_t adv_int_max = 0x0030;
  uint8_t adv_type = 0;
  bd_addr_t null_addr;
  memset(null_addr, 0, 6);
  gap_advertisements_set_params(adv_int_min, adv_int_max, adv_type, 0, null_addr, 0x07, 0x00);
  gap_advertisements_set_data(adv_data_len, (uint8_t*) adv_data);
  gap_advertisements_enable(1);

  // register for HCI events
  hci_event_callback_registration.callback = &packet_handler;
  hci_add_event_handler(&hci_event_callback_registration);

  // register for ATT event
  att_server_register_packet_handler(packet_handler);
}

// Code executed by BTStacks handlers is thread safe
// We *shouldnt* need to manually run code on core 0 or use btstack_run_loop_execute_on_main_thread in these handlers
static uint16_t att_read_callback(hci_con_handle_t connection_handle, uint16_t att_handle, uint16_t offset, uint8_t * buffer, uint16_t buffer_size){
  UNUSED(connection_handle);

  // This is our auto generated attribute UUID handle. This is how we check the read came from characteristic A vs B
  if (att_handle == ATT_CHARACTERISTIC_0000FF11_0000_1000_8000_00805F9B34FB_01_VALUE_HANDLE){
    // Get the value to send to the connected device by executing our onRead callback
    const char * response = onReadCallback();
    
    return att_read_callback_handle_blob((const uint8_t *)response, (uint16_t) strlen(response), offset, buffer, buffer_size);
  }

  return 0;
}

// Code executed by BTStacks handlers is thread safe
// We *shouldnt* need to manually run code on core 0 or use btstack_run_loop_execute_on_main_thread in these handlers
static int att_write_callback(hci_con_handle_t connection_handle, uint16_t att_handle, uint16_t transaction_mode, uint16_t offset, uint8_t *buffer, uint16_t buffer_size){
  UNUSED(transaction_mode);
  UNUSED(offset);
  UNUSED(buffer_size);
  UNUSED(connection_handle);    
  
  if (att_handle == ATT_CHARACTERISTIC_0000FF11_0000_1000_8000_00805F9B34FB_01_VALUE_HANDLE) {
    onWriteCallback(buffer);
  } else if (att_handle == ATT_CHARACTERISTIC_0000FF12_0000_1000_8000_00805F9B34FA_01_CLIENT_CONFIGURATION_HANDLE) {
    printf("config");
    le_notification_enabled = little_endian_read_16(buffer, 0) == GATT_CLIENT_CHARACTERISTICS_CONFIGURATION_NOTIFICATION;
    con_handle = connection_handle;
  }else if (att_handle == ATT_CHARACTERISTIC_0000FF12_0000_1000_8000_00805F9B34FA_01_VALUE_HANDLE) {
    //If it's the first packet of OTA since bootup, begin OTA
    //con_handle = connection_handle;
    if (!updateFlag) { 
        printf("\nBegin OTA");
        esp_ota_begin(esp_ota_get_next_update_partition(NULL), OTA_SIZE_UNKNOWN, &otaHandler);
        updateFlag = true;
    }

    //printf("On core: %d\n", buffer_size);

    if (buffer_size > 0)
    {
        esp_ota_write(otaHandler, buffer, buffer_size);

        // Consider the OTA complete if the block is smaller than the blocksize
        // There is an edge case if the update binary size is exactly a multple of 244
            printf("\nGot block");
        if (buffer_size != 244)
        {
            esp_ota_end(otaHandler);
            printf("\nEnd OTA");

            if (ESP_OK == esp_ota_set_boot_partition(esp_ota_get_next_update_partition(NULL))) {
                delay(2000);
                esp_restart();
            } else {
                printf("\nUpload Error");
            }
        }

        if (le_notification_enabled) {
          printf("Requesting Send");
          att_server_request_can_send_now_event(con_handle);
        }
    }
  }

  return 0;
}

static char counter_string[30];
static int  counter_string_len = snprintf(counter_string, sizeof(counter_string), "BTstack counter");

static void packet_handler (uint8_t packet_type, uint16_t channel, uint8_t *packet, uint16_t size){
    UNUSED(channel);
    UNUSED(size);

    if (packet_type != HCI_EVENT_PACKET) return;
    
    switch (hci_event_packet_get_type(packet)) {
        case HCI_EVENT_DISCONNECTION_COMPLETE:
            le_notification_enabled = 0;
            break;
        case ATT_EVENT_CAN_SEND_NOW:
          printf("SENDING");
            att_server_notify(con_handle, ATT_CHARACTERISTIC_0000FF12_0000_1000_8000_00805F9B34FA_01_VALUE_HANDLE, (uint8_t*) counter_string, counter_string_len);
            break;
        default:
            break;
    }
}
