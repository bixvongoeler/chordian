#include <Wire.h>
#include <pins_arduino.h>
#include <stdint.h>
#include "Adafruit_VL53L0X.h"
#include "1euroFilter.h"
#include <esp_now.h>
#include <WiFi.h>

/* Pin/Key Values */
#define DIG_PINS_IN_USE 10
#define NUM_ANALOG 16
#define NUM_KEYS 8

const uint8_t digitalPins[DIG_PINS_IN_USE] = { 15, 4, 16, 17, 5, 18, 19, 13, 26, 23 };

/* Rotary Encoder 1 */
#define ROTARY1_PIN1 34
#define ROTARY1_PIN2 35
#define RBUTTON1_PIN 27
/* Rotary Encoder 2 */
#define ROTARY2_PIN1 14
#define ROTARY2_PIN2 12
#define RBUTTON2_PIN 32

#define TOF_I2C_SDA 33
#define TOF_I2C_SCL 25

/* SDA Pin = 21, SCL Pin = 22 */
#define I2C_ADDRESS 0x08
uint8_t receiverMAC[] = { 0x88, 0x57, 0x21, 0x84, 0x86, 0xD0 };
Adafruit_VL53L0X lox = Adafruit_VL53L0X();

/* Filters */
#define FREQUENCY 27.03 // [Hz]
#define MINCUTOFF 0.003 // [Hz] needs to be tuned according to your application
#define BETA 0.0005 // needs to be tuned according to your application
static OneEuroFilter f;

#define VFREQUENCY 27.03 // [Hz]
#define VMINCUTOFF 0.007 // [Hz] needs to be tuned according to your application
#define VBETA 0.003 // needs to be tuned according to your application
static OneEuroFilter fv;

/* Structures */
struct analogData {
    uint16_t sensorForce[8]; // 8 Force Sensors
    uint16_t sensorPot[8]; // 8 Soft Pots
};

typedef struct {
    analogData analog_values;
    int8_t midi_cc_bellows;
    int8_t digital_keys[DIG_PINS_IN_USE];
} ChordianDataPacket;

/* Global Vars */
int val = 0;
unsigned long prev_time;
unsigned long start_time;
uint16_t prev_range;
ChordianDataPacket packet = { 0 };
uint8_t send_result = 0;

void onSent(const wifi_tx_info_t *info, esp_now_send_status_t status)
{
    send_result = ESP_NOW_SEND_SUCCESS ? 1 : 0;
    // Serial.println(status == ESP_NOW_SEND_SUCCESS ? "send_result:1," : "send_result:0,");
}

float mapSpeedToMidi(float speed)
{
    const float DEAD_ZONE = 3.0;
    const float SWEET_SPOT_SPEED = 15.0;
    const float MAX_SPEED = 120.0;

    if (speed < DEAD_ZONE)
        return 0;

    if (speed < SWEET_SPOT_SPEED) {
        // Log curve: fast rise, levels off approaching 75
        float t = (speed - DEAD_ZONE) / (SWEET_SPOT_SPEED - DEAD_ZONE);
        return log1p(t * 9.0) / log(10.0) * 75.0;
    } else {
        // Log curve: 75 to 127
        float t = (speed - SWEET_SPOT_SPEED) / (MAX_SPEED - SWEET_SPOT_SPEED);
        t = constrain(t, 0.0, 1.0);
        return 75.0 + log1p(t * 9.0) / log(10.0) * 52.0;
    }
}

void print_plotter_vals()
{
    Serial.print("t:");
    Serial.print(100);
    // Serial.printf("%.3f", velocity);
    Serial.print(",");

    Serial.print("z:");
    Serial.print(0);
    // Serial.printf("%.3f", velocity);
    Serial.print(",");

    Serial.print("b:");
    Serial.print(-100);
    // Serial.printf("%.3f", velocity);
    Serial.print(",");

    // Serial.print("range_mm:");
    // Seial.print(curr_range);
    // Serial.print(",");

    // Serial.print("delta_time:");
    // Serial.print(delta_time);
    // Serial.print(",");
    // Serial.print("elapsed_time:");
    // Serial.print(elapsed_time);
    // Serial.print(",");

    // Serial.print("filtered_range:");
    // Serial.print(filtered_range);
    // Serial.print(",");

    // Serial.print("velocity:");
    // Serial.print(velocity);
    // Serial.print(",");

    // Serial.print("filtered_velocity:");
    // Serial.print(filtered_velocity);
    // Serial.print(",");

    // Serial.print("midi_value:");
    // Serial.print(midi_value);
    // // Serial.printf("%.3f", velocity);
    // Serial.print(",");

    Serial.println(""); // new line after all readings are completed to format
}

/* Main Entry Point */
void setup(void)
{
    /********* Init Serial Buses */
    Serial.begin(115200);
    while (!Serial) {
        delay(1);
    }
    Serial.println("Chordian Booting: Serial Initialized...");

    /********* Set Pin Input Mode */
    // for (int i = 0; i < DIG_PINS_IN_USE; i++) {
    //     pinMode(digitalPins[i], INPUT_PULLUP); // Declare operation mode for the digital
    // }

    /********* Setup WiFi Connection *********/
    // Init ESP Now
    WiFi.mode(WIFI_STA);
    if (esp_now_init() != ESP_OK) {
        Serial.println("ESP-NOW init failed");
        return;
    }
    esp_now_register_send_cb(onSent);

    // Register peer
    esp_now_peer_info_t peer = {};
    memcpy(peer.peer_addr, receiverMAC, 6);
    peer.channel = 0;
    peer.encrypt = false;
    esp_now_add_peer(&peer);

    /********* Init TOF Sensor and Start Ranging *********/
    Wire.begin();
    bool booted = 0;
    while (!booted) {
        booted = lox.begin();
        if (!booted) {
            delay(50);
            Serial.println("Chordian Booting: TOF Boot Failed, Retrying...");
        }
    }
    delay(10);
    lox.startRangeContinuous();
    delay(10);
    Serial.println(F("Chordian Booting: TOF Booted and Ranging Successfully..."));

    /********* Init Filters */
    f.begin(FREQUENCY, MINCUTOFF, BETA);
    fv.begin(VFREQUENCY, VMINCUTOFF, VBETA);

    /* Set Initial Values */
    prev_time = micros();
    start_time = micros();
    // prev_range = 100;
    if (lox.isRangeComplete()) {
        prev_range = lox.readRange();
    } else {
        prev_range = 100;
    }
}

/* Main Loop */
void loop(void)
{
    unsigned long t_loop_start = micros();

    /********* Fetch Analog Values from Mega*/
    analogData analogValues = packet.analog_values;
    Wire.requestFrom(I2C_ADDRESS, sizeof(struct analogData));
    if (Wire.available() == sizeof(struct analogData)) {
        Wire.readBytes((byte *)&analogValues, sizeof(struct analogData));
    } else {
        Serial.print("\n WARNING: WIRE NOT AVAILABLE!\n");
    }
    packet.analog_values = analogValues;

    // Get all digital pin values and print to serial
    for (int i = 0; i < DIG_PINS_IN_USE; i++) {
        packet.digital_keys[i] = digitalRead(digitalPins[i]);
    }

    /* Try to fetch new distance value, reverting to prev on fail */
    uint16_t curr_range = prev_range;
    int attempts = 0;
    bool completed = lox.isRangeComplete();
    while ((!completed) && (attempts++ < 15)) {
        delay(1);
        completed = lox.isRangeComplete();
    }
    if (completed) {
        curr_range = lox.readRange();
    }

    unsigned long curr_time = micros();
    double delta_time = 1E-5 * (curr_time - prev_time);
    double elapsed_time = 1E-6 * (curr_time - start_time);

    double filtered_range = f.filter(curr_range, elapsed_time);

    double delta_range = filtered_range - prev_range;
    double velocity = (delta_range / (double)delta_time);
    double filtered_velocity = fv.filter(abs(velocity), elapsed_time);

    float midi_value = mapSpeedToMidi(filtered_velocity);
    packet.midi_cc_bellows = (int8_t)midi_value;
    // packet.midi_cc_bellows = 88;

    prev_range = filtered_range;
    prev_time = curr_time;

    Serial.print("| Forces: ");
    for (int i = 0; i < 8; i++) {
        Serial.printf("%04d ", packet.analog_values.sensorForce[i]);
    }
    Serial.print("| Pots: ");
    for (int i = 0; i < 8; i++) {
        Serial.printf("%04d ", packet.analog_values.sensorPot[i]);
    }
    Serial.print("| Keys: ");
    for (int i = 0; i < DIG_PINS_IN_USE; i++) {
        Serial.printf("%01d ", packet.digital_keys[i]);
    }
    unsigned long t_loop = (micros() - t_loop_start);
    Serial.printf("| Tof:%06.2f | Sent:%d | T_Loop:%03d\n", midi_value,
                  (send_result ? 127 : 0), t_loop);

    // for (int i = 0; i < 3; i++) {
    //     Serial.printf("F%d:%04d,", i,  packet.analog_values.sensorForce[i]);
    // }
    // for (int i = 0; i < 3; i++) {
    //     Serial.printf("P%d:%04d,", i, packet.analog_values.sensorPot[i]);
    // }
    // unsigned long t_loop = (micros() - t_loop_start);
    // Serial.printf("TOF:%06.2f,SENT:%d,T_LOOP:%03d\n", midi_value, (send_result ? 127 : 0), t_loop);

    /******** Send Packet to Receiver ********/
    esp_now_send(receiverMAC, (uint8_t *)&packet, sizeof(packet));
}
