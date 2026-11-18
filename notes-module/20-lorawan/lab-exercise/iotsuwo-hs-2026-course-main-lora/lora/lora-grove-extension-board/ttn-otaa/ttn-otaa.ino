/*******************************************************************************
 * This example sends a valid LoRaWAN packet with a payload, 
 * using frequency and encryption settings matching those of
 * the The Things Network.
 *
 * This uses OTAA (Over-the-air activation), where where a DevEUI and
 * application key is configured, which are used in an over-the-air
 * activation procedure where a DevAddr and session keys are
 * assigned/generated for use with all further communication.
 *
 * Note: LoRaWAN per sub-band duty-cycle limitation is enforced (1% in
 * g1, 0.1% in g2), but not the TTN fair usage policy (which is probably
 * violated by this sketch when left running for longer)!

 * To use this prototype, first register your application and device with
 * the things network, to set or generate an AppEUI, DevEUI and AppKey.
 * Multiple devices can use the same AppEUI, but each device has its own
 * DevEUI and AppKey.
 *
 *******************************************************************************/

#include <lmic.h>
#include <hal/hal.h>
#include <SPI.h>

#include "Adafruit_CCS811.h"
#include "Adafruit_Si7021.h"

Adafruit_CCS811 co2Sensor;
Adafruit_Si7021 tempHumSensor = Adafruit_Si7021();

const long dataUpdateInterval = 10000; // update sensor data every 10s

long startTime = 0;

long co2Sum = 0;
int co2ReadCount = 0;
int humiditySum = 0;
int temperatureSum  = 0;
int humTempReadCount = 0;

uint8_t message[6];

// AppEUI=70B3D57ED0015FC3
// This EUI must be in little-endian format, so least-significant-byte
// first. When copying an EUI from ttnctl output, this means to reverse
// the bytes. For TTN issued EUIs the last bytes should be 0xD5, 0xB3,
// 0x70.
//static const u1_t PROGMEM APPEUI[8]={ 0xC3, 0x5F, 0x01, 0xD0, 0x7E, 0xD5, 0xB3, 0x70 };

// AppEUI=0000000000000000
static const u1_t PROGMEM APPEUI[8]={ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };


void os_getArtEui (u1_t* buf) { memcpy_P(buf, APPEUI, 8);}

// DevEUI=004DB1E0A39734AC
// This should also be in little endian format, see above.
//static const u1_t PROGMEM DEVEUI[8]={ 0xAC, 0x34, 0x97, 0xA3, 0xE0, 0xB1, 0x4D, 0x00 };

// DevEUI= 70B3D57ED004FCF9
static const u1_t PROGMEM DEVEUI[8]={ 0xF9, 0xFC, 0x04, 0xD0, 0x7E, 0xD5, 0xB3, 0x70 };


void os_getDevEui (u1_t* buf) { memcpy_P(buf, DEVEUI, 8);}

// AppKey=BE4C78E491244171D3C7374FF4E56AC6
// This key should be in big endian format (or, since it is not really a
// number but a block of memory, endianness does not really apply). In
// practice, a key taken from ttnctl can be copied as-is.
// The key shown here is the semtech default key.
//static const u1_t PROGMEM APPKEY[16] ={ 0xBE, 0x4C, 0x78, 0xE4, 0x91, 0x24, 0x41, 0x71, 0xD3, 0xC7, 0x37, 0x4F, 0xF4, 0xE5, 0x6A, 0xC6 };

// AppKey= BE 75 6F E8 1B 7A 8E 37 99 86 3C CC 6A C2 97 F3
static const u1_t PROGMEM APPKEY[16] ={ 0xBE, 0x75, 0x6F, 0xE8, 0x1B, 0x7A, 0x8E, 0x37, 0x99, 0x86, 0x3C, 0xCC, 0x6A, 0xC2, 0x97, 0xF3 };


void os_getDevKey (u1_t* buf) {  memcpy_P(buf, APPKEY, 16);}

static osjob_t sendjob;

// Schedule TX every this many seconds (might become longer due to duty
// cycle limitations).
const unsigned TX_INTERVAL = 60;

// Pin mapping
const lmic_pinmap lmic_pins = {
    .nss = 10,
    .rxtx = LMIC_UNUSED_PIN,
    .rst = LMIC_UNUSED_PIN,
    .dio = {2, 8, LMIC_UNUSED_PIN},
};

void onEvent (ev_t ev) {
    Serial.print(os_getTime());
    Serial.print(": ");
    switch(ev) {
        case EV_SCAN_TIMEOUT:
            Serial.println(F("EV_SCAN_TIMEOUT"));
            break;
        case EV_BEACON_FOUND:
            Serial.println(F("EV_BEACON_FOUND"));
            break;
        case EV_BEACON_MISSED:
            Serial.println(F("EV_BEACON_MISSED"));
            break;
        case EV_BEACON_TRACKED:
            Serial.println(F("EV_BEACON_TRACKED"));
            break;
        case EV_JOINING:
            Serial.println(F("EV_JOINING"));
            break;
        case EV_JOINED:
            Serial.println(F("EV_JOINED"));

            // Disable link check validation (automatically enabled
            // during join, but not supported by TTN at this time).
            LMIC_setLinkCheckMode(0);
            break;
        case EV_RFU1:
            Serial.println(F("EV_RFU1"));
            break;
        case EV_JOIN_FAILED:
            Serial.println(F("EV_JOIN_FAILED"));
            break;
        case EV_REJOIN_FAILED:
            Serial.println(F("EV_REJOIN_FAILED"));
            break;
            break;
        case EV_TXCOMPLETE:
            Serial.println(F("EV_TXCOMPLETE (includes waiting for RX windows)"));
            if (LMIC.txrxFlags & TXRX_ACK)
              Serial.println(F("Received ack"));
            if (LMIC.dataLen) {
              Serial.println(F("Received "));
              Serial.println(LMIC.dataLen);
              Serial.println(F(" bytes of payload"));
            }
            // Schedule next transmission
            os_setTimedCallback(&sendjob, os_getTime()+sec2osticks(TX_INTERVAL), do_send);
            break;
        case EV_LOST_TSYNC:
            Serial.println(F("EV_LOST_TSYNC"));
            break;
        case EV_RESET:
            Serial.println(F("EV_RESET"));
            break;
        case EV_RXCOMPLETE:
            // data received in ping slot
            Serial.println(F("EV_RXCOMPLETE"));
            break;
        case EV_LINK_DEAD:
            Serial.println(F("EV_LINK_DEAD"));
            break;
        case EV_LINK_ALIVE:
            Serial.println(F("EV_LINK_ALIVE"));
            break;
         default:
            Serial.println(F("Unknown event"));
            break;
    }
}

// Method that is called every time constant TX_INTERVAL is reached
void do_send(osjob_t* j){ 
    // Check if there is not a current TX/RX job running
    if (LMIC.opmode & OP_TXRXPEND) {
        Serial.println(F("OP_TXRXPEND, not sending"));
    } else {
        // Prepare upstream data transmission at the next possible time. 

        // to eliminate spikes and the likes the data is smoothed out over time before it's being sent out
        int humi = 0;
        int temp = 0;

        if (humTempReadCount > 0) {
            humi = humiditySum / humTempReadCount * 100; // * 100 for 2 decimal digits
            temp = temperatureSum / humTempReadCount * 100; // * 100 for 2 decimal digits
        }

        int co2ppm = 0;
        
        // calculate average of all co2 readings since last transmit
        if (co2ReadCount > 0) {
            Serial.print("co2sum: ");
            Serial.println(co2Sum);
            Serial.print("Readcount: ");
            Serial.println(co2ReadCount);
            co2ppm = co2Sum / co2ReadCount; 
        }
        
        // split 16 bit int into 8 bit uint8 parts
        message[0] = highByte(temp);
        message[1] = lowByte(temp);
        message[2] = highByte(humi);
        message[3] = lowByte(humi);
        message[4] = highByte(co2ppm);
        message[5] = lowByte(co2ppm);
              
        LMIC_setTxData2(1, message, sizeof(message), 0);
        Serial.println(F("Packet queued"));

        co2ReadCount = 0; // reset co2 readings counter
        humTempReadCount = 0; // reset humidity and temperature readings counter
        humiditySum = 0;
        temperatureSum =  0;
        co2Sum = 0;
    }
    // Next TX is scheduled after TX_COMPLETE event.
}

void setup() {
    Serial.begin(9600);
    Serial.println(F("Starting"));

    if(!tempHumSensor.begin()) {
        Serial.println("Failed to start Si7021 sensor!");
        while (1);
    }

    if(!co2Sensor.begin()){
       Serial.println("Failed to start CCS811 sensor!");
       while(1);
    }

    //set drive mode to one reading every 60s
    co2Sensor.setDriveMode(0x03);
    co2Sensor.setTempOffset(tempHumSensor.readTemperature() - 25.0);

    #ifdef VCC_ENABLE
    // For Pinoccio Scout boards
    pinMode(VCC_ENABLE, OUTPUT);
    digitalWrite(VCC_ENABLE, HIGH);
    delay(1000);
    #endif

    // LMIC init
    os_init();
    // Reset the MAC state. Session and pending data transfers will be discarded.
    LMIC_reset();

    // Start job (sending automatically starts OTAA too)
    do_send(&sendjob);

    startTime = millis();

}

void loop() {
    os_runloop_once();
    
    if (millis() - startTime > dataUpdateInterval) {

            // every 60s new data is available on co2 sensor 
            if(!co2Sensor.readData()){
                int co2Reading = co2Sensor.geteCO2();
                Serial.print("CO2 = ");
                Serial.println(co2Reading);
                if(co2Reading > 400) { // realistic  value (startup phase done)
                    co2Sum = co2Sum + co2Reading;
                    co2ReadCount = co2ReadCount + 1;
                }

            } else {
                Serial.println("ERROR reading CO2 Sensor!");
                while(1);
            }   
            
        
    
        // read humidity and temperature and save it
        humiditySum = humiditySum + tempHumSensor.readHumidity();
        Serial.print("Humidity = ");
        Serial.println(tempHumSensor.readHumidity());
        temperatureSum = temperatureSum + tempHumSensor.readTemperature();
        Serial.print("Temperature = ");
        Serial.println(tempHumSensor.readTemperature());
        humTempReadCount = humTempReadCount + 1;
        
        startTime = millis(); // reset startTime 
    }
}
