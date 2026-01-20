#include <esp_now.h>
#include <WiFi.h>
#include <Wire.h>
#include <stdint.h>

// Receiver MAC: 88:57:21:84:86:D0

#define I2C_ADDRESS 0x08
#define NUM_ANALOG 16
#define NUM_KEYS 8
#define DIG_PINS_IN_USE 12

struct analogData {
    uint16_t sensorForce[8]; // 8 Force Sensors
    uint16_t sensorPot[8]; // 8 Soft Pots
};

typedef struct {
    analogData analog_values;
    int8_t midi_cc_bellows;
    int8_t digital_keys[DIG_PINS_IN_USE];
} ChordianDataPacket;

void packet_to_max(ChordianDataPacket *packet) {
    analogData *av = &packet->analog_values;
    for (int i = 0; i < NUM_KEYS; i++) {
        Serial.print(av->sensorForce[i]);
        Serial.print(" ");
    }
    for (int i = 0; i < NUM_KEYS; i++) {
        Serial.print(av->sensorPot[i]);
        Serial.print(" ");
    }
    for (int i = 0; i < DIG_PINS_IN_USE; i++) {
        Serial.print(packet->digital_keys[i]);
        Serial.print(" ");
    }
    Serial.print(packet->midi_cc_bellows);
    Serial.println("");
}

void onReceive(const esp_now_recv_info_t *info, const uint8_t *data, int len) {
    ChordianDataPacket *packet = (ChordianDataPacket *)data;
    packet_to_max(packet);
    // Serial.printf("pot:%d,midi_cc:%d\n", packet->analog_values.sensorPot[0], packet->midi_cc_bellows);
}

void setup() {
    Serial.begin(115200);
    while (!Serial) {
        delay(1);
    }
    Serial.println("Receiver Booting...");
    WiFi.mode(WIFI_STA);
    WiFi.STA.begin();  // Actually starts the WiFi driver
    
    Serial.print("Receiver MAC: ");
    Serial.println(WiFi.macAddress());
    
    if (esp_now_init() != ESP_OK) {
        Serial.println("ESP-NOW init failed");
        return;
    }
    esp_now_register_recv_cb(onReceive);
}

void loop()
{
}
