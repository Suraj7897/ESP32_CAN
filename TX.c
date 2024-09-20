#include <OneWire.h>
#include <DallasTemperature.h>
#include <SPI.h>
#include <mcp2515.h>
 
#define ONE_WIRE_BUS 21 // GPIO where the DS18B20 is connected
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);
 
struct can_frame canMsg;
struct MCP2515 mcp2515(5); // CS pin is GPIO 5
 
#define MAX_RETRIES 3
#define CAN_ACK_ID 0x037  // CAN ID for acknowledgment
 
void setup() {
  Serial.begin(115200);
  SPI.begin();
  mcp2515.reset();
  mcp2515.setBitrate(CAN_500KBPS, MCP_8MHZ);
  mcp2515.setNormalMode();
 
  sensors.begin();
}
 
void loop() {
  sensors.requestTemperatures(); 
  float temperatureC = sensors.getTempCByIndex(0);
  int tempInt = (int)(temperatureC * 100); // Convert temperature to integer for transmission
  
  // Prepare CAN message
  canMsg.can_id  = 0x036;  // CAN ID
  canMsg.can_dlc = 2;      // Data length code (number of bytes)
  canMsg.data[0] = (tempInt >> 8) & 0xFF; // MSB of temperature
  canMsg.data[1] = tempInt & 0xFF;        // LSB of temperature
 
  bool messageSent = false;
  int retries = 0;
 
  while (!messageSent && retries < MAX_RETRIES) {
    if (mcp2515.sendMessage(&canMsg) == MCP2515::ERROR_OK) {
      Serial.print("Temperature sent: ");
      Serial.print(temperatureC);
      Serial.println(" °C");
 
      // Wait for acknowledgment
      unsigned long startTime = millis();
      bool ackReceived = false;
      
      while (millis() - startTime < 500) { // Wait up to 500ms for an ACK
        if (mcp2515.readMessage(&canMsg) == MCP2515::ERROR_OK) {
          if (canMsg.can_id == CAN_ACK_ID) {
            ackReceived = true;
            break;
          }
        }
      }
 
      if (ackReceived) {
        Serial.println("ACK received");
        messageSent = true;
      } else {
        Serial.println("ACK not received, retrying...");
        retries++;
      }
    } else {
      Serial.println("Error sending message, retrying...");
      retries++;
    }
  }
 
  if (!messageSent) {
    Serial.println("Failed to send message after retries");
  }
 
  delay(1000); // Send data every second
}