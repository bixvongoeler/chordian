#include <Wire.h>
#include <stdint.h>

#define I2C_ADDRESS 0x08
#define NUM_ANALOG 16

const uint8_t analogPins[16] = { A0, A1, A2,  A3,  A4,  A5,  A6,  A7,
                                 A8, A9, A10, A11, A12, A13, A14, A15 };

struct analogData {
    uint16_t sensorForce[8]; // 8 Force Sensors
    uint16_t sensorPot[8]; // 8 Soft Pots
};

analogData transfer_values = { 0 };

void requestEvent()
{
    Wire.write((byte *)&transfer_values, sizeof(transfer_values));
}

void setup()
{
    Serial.begin(115200);
    while (!Serial) {
        delay(1);
    }
    Serial.println("Started Peripheral Mega!");

    Wire.begin(I2C_ADDRESS);
    Wire.onRequest(requestEvent);
    delay(10);
    Serial.println("Starting Loop!");
}

void loop()
{
    unsigned long t_loop_start = micros();
    for (int i = 0; i < NUM_ANALOG; i++) {
        if (i < 8) {
            transfer_values.sensorForce[i] = analogRead(analogPins[i]);
        } else {
            transfer_values.sensorPot[i - 8] = analogRead(analogPins[i]);
        }
    }
    Serial.print("Forces: ");
    for (int i = 0; i < 8; i++) {
        Serial.print(transfer_values.sensorForce[i]);
        Serial.print(" ");
    }
    Serial.print("| Pots: ");
    for (int i = 0; i < 8; i++) {
        Serial.print(transfer_values.sensorPot[i]);
        Serial.print(" ");
    }
    Serial.print("| T_loop: ");
    Serial.print((micros() - t_loop_start));
    Serial.println("");
}
