#include <CurieBLE.h>

BLEService statusService("cb73af71-04cf-4938-99df-756fb8f7cbb9");
BLECharacteristic stateChar("8573d3e8-c84f-4efe-8cbf-0dd72b7ac1f7", BLERead | BLEWrite, 5);

uint8_t state[] = {0x00, 0x00, 0x00, 0x00, 0x00};


void setup() {
  Serial.begin(9600);    // initialize serial communication

  BLE.begin();

  BLE.setLocalName("BB");
  BLE.setAdvertisedService(statusService);
  statusService.addCharacteristic(stateChar);
  BLE.addService(statusService);
  BLE.advertise();
}

void loop() {
  BLEDevice central = BLE.central();

  if (central) {
    Serial.print("Connected to central: ");
    Serial.println(central.address());

    while (central.connected()) {
     if (stateChar.written()) {
        const byte request = stateChar.value()[0];

        if (request == 1) {
          Serial.println("Got GET_STATE request!");
          state[0] = 13;
          state[1] = 3;
          state[2] = 20;
          state[3] = 71;
          state[4] = 101;
          stateChar.setValue(state, 5);
        } else if (request == 2) {
          Serial.println("Got RESET request!");
        } else {
          Serial.print("Got unknown request: ");
          Serial.println(request);
        }

      }
    }

    Serial.print("Disconnected from central: ");
    Serial.println(central.address());
  }
}
