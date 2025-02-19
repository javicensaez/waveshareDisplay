#include "waveshare_twai_port.h"
#include <ui.h>

 unsigned long previousMillis = 0; 
 #define TRANSMIT_RATE_MS 1000

// Function to handle received messages
static void handle_rx_message(twai_message_t &message)
{
  // Process received message
  if (message.extd)
  {
    Serial.println("Message is in Extended Format"); // Print if the message is in extended format
    lv_textarea_add_text(ui_TextArea1, "Message is in Extended Format");
  }
  else
  {
    Serial.println("Message is in Standard Format"); // Print if the message is in standard format
  }
  Serial.printf("ID: %x\nByte:", message.identifier); // Print message ID
  if (!(message.rtr))
  { // Check if it is not a remote transmission request
    for (int i = 0; i < message.data_length_code; i++)
    {
      Serial.printf(" %d = %02x,", i, message.data[i]); // Print each byte of the message data
    }
    Serial.println(""); // Print a new line
    lv_arc_set_value(ui_Arc1, message.data[0]);
  }
}

// Function to initialize the TWAI driver
bool waveshare_twai_init()
{
  Serial.println("Driver installing");
  // Initialize configuration structures using macro initializers
  twai_general_config_t g_config = TWAI_GENERAL_CONFIG_DEFAULT((gpio_num_t)TX_PIN, (gpio_num_t)RX_PIN, TWAI_MODE_NORMAL);
  twai_timing_config_t t_config = TWAI_TIMING_CONFIG_250KBITS();   // set 250Kbps
  twai_filter_config_t f_config = TWAI_FILTER_CONFIG_ACCEPT_ALL(); // Accept all messages

  // Install TWAI driver
  if (twai_driver_install(&g_config, &t_config, &f_config) != ESP_OK)
  {
    Serial.println("Failed to install driver"); // Print error message
    return false;                               // Return false if driver installation fails
  }
  Serial.println("Driver installed"); // Print success message

  // Start TWAI driver
  if (twai_start() != ESP_OK)
  {
    Serial.println("Failed to start driver"); // Print error message
    return false;                             // Return false if starting the driver fails
  }
  Serial.println("Driver started"); // Print success message

  // Reconfigure alerts to detect frame receive, Bus-Off error, and RX queue full states
  uint32_t alerts_to_enable = TWAI_ALERT_RX_DATA | TWAI_ALERT_ERR_PASS | TWAI_ALERT_BUS_ERROR | TWAI_ALERT_RX_QUEUE_FULL | TWAI_ALERT_TX_IDLE | TWAI_ALERT_TX_SUCCESS | TWAI_ALERT_TX_FAILED | TWAI_ALERT_ERR_PASS | TWAI_ALERT_BUS_ERROR;
  if (twai_reconfigure_alerts(alerts_to_enable, NULL) == ESP_OK)
  {
    Serial.println("CAN Alerts reconfigured"); // Print success message
  }
  else
  {
    Serial.println("Failed to reconfigure alerts"); // Print error message
    return false;                                   // Return false if alert reconfiguration fails
  }

  // TWAI driver is now successfully installed and started
  return true; // Return true on success
}

// Function to receive messages via TWAI
void waveshare_twai_receive()
{
  // Check if alert happened
  uint32_t alerts_triggered;
  twai_read_alerts(&alerts_triggered, pdMS_TO_TICKS(POLLING_RATE_MS)); // Read triggered alerts
  twai_status_info_t twaistatus;                                       // Create status info structure
  twai_get_status_info(&twaistatus);                                   // Get status information

  // Handle alerts
  if (alerts_triggered & TWAI_ALERT_ERR_PASS)
  {
    Serial.println("Alert: TWAI controller has become error passive."); // Print passive error alert
  }
  if (alerts_triggered & TWAI_ALERT_BUS_ERROR)
  {
    Serial.println("Alert: A (Bit, Stuff, CRC, Form, ACK) error has occurred on the bus."); // Print bus error alert
    Serial.printf("Bus error count: %d\n", twaistatus.bus_error_count);                     // Print bus error count
  }
  if (alerts_triggered & TWAI_ALERT_RX_QUEUE_FULL)
  {
    Serial.println("Alert: The RX queue is full causing a received frame to be lost."); // Print RX queue full alert
    Serial.printf("RX buffered: %d\t", twaistatus.msgs_to_rx);                          // Print buffered RX messages
    Serial.printf("RX missed: %d\t", twaistatus.rx_missed_count);                       // Print missed RX count
    Serial.printf("RX overrun %d\n", twaistatus.rx_overrun_count);                      // Print RX overrun count
  }

  // Check if message is received
  if (alerts_triggered & TWAI_ALERT_RX_DATA)
  {
    // One or more messages received. Handle all.
    twai_message_t message;
    while (twai_receive(&message, 0) == ESP_OK)
    {                             // Receive messages
      handle_rx_message(message); // Handle each received message

    }
  }
}

// Send message
static void send_message() {
  // Configure message to transmit
  twai_message_t message;
  message.identifier = 0x0F6; // Set message identifier
  message.data_length_code = 8; // Set data length
  for (int i = 0; i < message.data_length_code; i++) {
    message.data[i] = i; // Populate message data
  }
  // Queue message for transmission 
  if (twai_transmit(&message, pdMS_TO_TICKS(1000)) == ESP_OK) {
    printf("Message queued for transmission\n"); // Print success message
  } else {
    printf("Failed to queue message for transmission\n"); // Print failure message
  }
  memset(message.data, 0, sizeof(message.data)); // Clear the entire array
}

void waveshare_twai_transmit()
{
    // Check if alert happened
    uint32_t alerts_triggered;
    twai_read_alerts(&alerts_triggered, pdMS_TO_TICKS(POLLING_RATE_MS)); // Read triggered alerts
    twai_status_info_t twaistatus; // Create status info structure
    twai_get_status_info(&twaistatus); // Get status information

    // Handle alerts
    if (alerts_triggered & TWAI_ALERT_ERR_PASS) {
      Serial.println("Alert: TWAI controller has become error passive."); // Print passive error alert
    }
    if (alerts_triggered & TWAI_ALERT_BUS_ERROR) {
      Serial.println("Alert: A (Bit, Stuff, CRC, Form, ACK) error has occurred on the bus."); // Print bus error alert
      Serial.printf("Bus error count: %d\n", twaistatus.bus_error_count); // Print bus error count
    }
    if (alerts_triggered & TWAI_ALERT_TX_FAILED) {
      Serial.println("Alert: The Transmission failed."); // Print transmission failure alert
      Serial.printf("TX buffered: %d\t", twaistatus.msgs_to_tx); // Print buffered TX messages
      Serial.printf("TX error: %d\t", twaistatus.tx_error_counter); // Print TX error count
      Serial.printf("TX failed: %d\n", twaistatus.tx_failed_count); // Print failed TX count
    }
    if (alerts_triggered & TWAI_ALERT_TX_SUCCESS) {
      Serial.println("Alert: The Transmission was successful."); // Print successful transmission alert
      Serial.printf("TX buffered: %d\t", twaistatus.msgs_to_tx); // Print buffered TX messages
    }

    // Send message
    unsigned long currentMillis = millis(); // Get current time in milliseconds
    if (currentMillis - previousMillis >= TRANSMIT_RATE_MS) { // Check if it's time to send a message
      previousMillis = currentMillis; // Update last send time
      send_message(); // Call function to send message
    }
}



