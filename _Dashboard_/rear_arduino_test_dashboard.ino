#include <SPI.h>
#include <mcp2515.h>

// CS pin changed to 53 for Arduino Mega
MCP2515 mcp2515(53);  // CS pin 53 (SPI chip select MCP2515 ke liye)

// Har sensor ke liye alag CAN ID use kar rahe hain
#define CAN_ID_SPEED          0x100
#define CAN_ID_RPM_LEFT       0x200
#define CAN_ID_RPM_RIGHT      0x201
#define CAN_ID_ACCUM_VOLTAGE  0x300
#define CAN_ID_LV_BATTERY     0x400
#define CAN_ID_SOC            0x500

// Ye structure CAN ka ek packet represent karega hai
struct can_frame frame;
// Frame matlab ye pura 8 bytes ka packet jo CAN bus par bheja jayega
// Structure me 3 main cheeze li:
// 1 Address (CAN ID)
// 2 Length (kitne bytes bhej rahe hain)
// 3 Data values

void sendFrame(uint32_t id, uint8_t len, uint8_t* inputdata){
    frame.can_id  = id;
    frame.can_dlc = len;
    memset(frame.data, 0, 8);
    
    // Data copy to frame
    for (int i = 0; i < len; i++) {
        frame.data[i] = inputdata[i];
    }
    if (mcp2515.sendMessage(&frame) == MCP2515::ERROR_OK) {
        Serial.print("Sent ID: 0x"); // kaunsa message gaya
        Serial.print(id, HEX);

        Serial.print(" | Data: "); // kaunsi value gayi
        for (int i = 0; i < len; i++) {
            Serial.print(inputdata[i]);
            Serial.print(" ");
        }

        Serial.println();

    } else {
        Serial.print("Failed to send ID: 0x");
        Serial.println(id, HEX);
    }
}


void setup() {

    Serial.begin(115200);

    SPI.begin();           // 🔥 VERY IMPORTANT
    delay(200);            // 🔥 give SPI time

    mcp2515.reset();
    delay(200);            // 🔥 give MCP2515 time

    randomSeed(analogRead(0));

    if (mcp2515.setBitrate(CAN_500KBPS, MCP_8MHZ) == MCP2515::ERROR_OK) {

        Serial.println("CAN Init OK");

    } else {

        Serial.println("CAN Init Failed");
        while (1);
    }

    mcp2515.setNormalMode();
    Serial.println("Ready to send...\n");
}


void loop() {
    uint8_t  speed         = random(0,150);
    uint16_t rpm_left      = random(400,1200);
    uint16_t rpm_right     = random(400,1200); 
    uint16_t accum_voltage = random(3000,4000); // Scalling kyuki decimal nahi bhej skte...
    uint8_t  lv_battery    = random(110,146); // Saclling
    uint8_t  soc           = random(0,101);
// Speed
    uint8_t speed_data[1] = { speed };
    sendFrame(CAN_ID_SPEED, 1, speed_data); // max 150 hence 1 byte enough 
    // Ya fir size ke liye sizeof(speed_data) bhi use kr skte hai
    delay(10);
    
// RPM Left
    uint8_t rpm_left_data[2] = {
        (rpm_left >> 8) & 0xFF,
        rpm_left & 0xFF 
    };
    sendFrame(CAN_ID_RPM_LEFT, 2, rpm_left_data); // max 1200 to 2 byte
    delay(10);
    
// RPM Right
    uint8_t rpm_right_data[2] = {

        (rpm_right >> 8) & 0xFF,
        rpm_right & 0xFF
    };
    sendFrame(CAN_ID_RPM_RIGHT, 2, rpm_right_data); // max 1200 to 2 byte
    delay(10);
    
// Acc. Voltage
    uint8_t voltage_data[2] = {
        (accum_voltage >> 8) & 0xFF,
        accum_voltage & 0xFF
    };
    sendFrame(CAN_ID_ACCUM_VOLTAGE, 2, voltage_data); // Upto 4000 to 2 bytes inhe join krna 
    delay(10);
// LV battery Voltage
    uint8_t lv_data[1] = { lv_battery };
    sendFrame(CAN_ID_LV_BATTERY, 1, lv_data); // Max 15 to 1 byte enough
    delay(10);
    
// Acc. SoC
    uint8_t soc_data[1] = { soc };
    sendFrame(CAN_ID_SOC, 1, soc_data);
    Serial.println("--------------------");

    delay(100); // Total delay 10x5 + 100 mili second hua = 150ms
}
