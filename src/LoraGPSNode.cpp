
#include "LoraGPSNode.h"

// --- Notes --- //
//Lora and debug settings are in .pio/libdeps/d1_mini/LMIC-Arduino/src/lmic/config.h


// --- Settings --- //
static const int GPS_RXPin = 5, GPS_TXPin = 4; //GPS Serial GPIO pin numbers
static const uint32_t GPSBaud = 9600; //GPS Baud Rate
const unsigned Lora_TX_INTERVAL = 60; // Schedule TX every this many seconds (might become longer due to duty cycle limitations).
unsigned long getGpsSignalTimeout = 120*1000; //milliseconds before giving up geting a valid GPS signal
unsigned long waitingForDataNotificationTimeout = 1*1000; //milliseconds

// ESP8266 LORA NODE V1.3 pinout. For further info, see https://github.com/hallard/WeMos-Lora
const lmic_pinmap lmic_pins = {
    .nss = 16, //GPIO16
    .rxtx = LMIC_UNUSED_PIN, //Unused
    .rst = LMIC_UNUSED_PIN, //Unused
    .dio = {15, 15, LMIC_UNUSED_PIN}, //GPIO15 for DIO 1 and 2.
};

//Complete these credentials or create a credentials.h file
#ifndef CREDENTIALS
    static const u1_t NWKSKEY[16] PROGMEM = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
    static const u1_t APPSKEY[16] PROGMEM = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
    static const u4_t DEVADDR = 0x00000000;
#endif

// --- Global Variables --- //
TinyGPSPlus gps; // The TinyGPS++ object for parsing GPS data
SoftwareSerial gpsSerial(GPS_RXPin, GPS_TXPin); // The serial connection to the GPS device
String gpsDataToSend; //The text of the data you wish to send over the Lora network
String defaultGpsData = ""; //If no data is received from the GPS module, send this text instead

// --- Code --- //

// These callbacks are only used in over-the-air activation, so they are
// left empty here (we cannot leave them out completely unless
// DISABLE_JOIN is set in config.h, otherwise the linker will complain).
void os_getArtEui (u1_t* buf) { }
void os_getDevEui (u1_t* buf) { }
void os_getDevKey (u1_t* buf) { }

static osjob_t sendjob;

//Receive

String getGPSData()
{
    Serial.println("Getting GPS Data...");
    String gpsData = defaultGpsData; //Stores the GPS data received from the GPS module.  Set a default text if no data is received within the set timeframe..
    unsigned long loopStartTime = millis(); //Used to track elapsed time waiting for GPS data
    unsigned long gpsWarningStartTime = millis(); //Used to provide a note if no GPS data has been received after 10 seconds
    while((gpsData == defaultGpsData) && (loopStartTime + getGpsSignalTimeout >= millis()) && (millis() >= loopStartTime)) //Wait up to the timeout delay for GPS data.  Also breaks the loop if overflow condition on millis() occurs (which will occur every 1.6 months).
    {
        //If data has been received (ie in buffer) and while it is still being received, feed the encode method
        while (gpsSerial.available() > 0)
        {
            gps.encode(gpsSerial.read());
            delay(1); //Prevents watchdog timer from timing out, and provides a wait for additional data to ensure entire string is captured.
        }
        
        //If GPS module provided lat/long coordinates.  Otherwise, it is likely still waiting for more satellites (Needs minimum 3 for results to triangulate)
        if (gps.location.isValid())
        {
            gpsData = String(gps.location.lat(),6) + ", " + String(gps.location.lng(),6);
            Serial.println("GPSData: " + gpsData);
        }
        if ((millis() - waitingForDataNotificationTimeout > 10000) && (gps.charsProcessed() < 10))
        {
            Serial.println("Waiting for GPS to produce data.");
            waitingForDataNotificationTimeout = 0;
        }
        //Provide a note if it's been 10 seconds and nothing at all has been received by the GPS
        if ((millis() - gpsWarningStartTime > 10000) && (gps.charsProcessed() < 10))
        {
            Serial.println("No GPS detected: check wiring.");
            gpsWarningStartTime = 0; //Restart so this message shows up every 10 seconds.
        }
    }
    if (gpsData == defaultGpsData)
    {
        Serial.println("Timed out waiting for valid GPS data after" + String((millis() - loopStartTime)/(int)1000) + " seconds.");
    }
    else
    {
        Serial.println("Valid GPS data received in " + String((millis() - loopStartTime)/(int)1000) + " seconds.");
    }
    return gpsData;
}

void do_send(osjob_t* j){
    gpsDataToSend = getGPSData();
    // Check if there is not a current TX/RX job running
    if (LMIC.opmode & OP_TXRXPEND) {
        Serial.println(F("OP_TXRXPEND, not sending"));
    } else {
        if (gpsDataToSend == defaultGpsData) //if GPSData was not changed
        {
            Serial.print("No GPS Data received, so not sending data.");
        }
        else //Send Data
        {
            unsigned char * gpsDataToSendAsChars = (unsigned char*)gpsDataToSend.c_str(); // Convert to unsigned char *, required for LMIC

            //Print out what is being sent
            Serial.print('\n');
            Serial.println("Sending: " + gpsDataToSend);
            Serial.print("As a char string: ");
            Serial.printf("%s",gpsDataToSendAsChars);
            Serial.print('\n');

            // Prepare upstream data transmission at the next possible time.
            LMIC_setTxData2(1, gpsDataToSendAsChars, gpsDataToSend.length(), true); //Send data over Lora Network and require an acknowledgement
            Serial.println(F("Packet queued for sending"));
        }
    }
    // Next TX is scheduled after TX_COMPLETE event.
}

//On a Lora Event
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
            os_setTimedCallback(&sendjob, os_getTime()+sec2osticks(Lora_TX_INTERVAL), do_send);
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

void setup() {
    Serial.begin(115200);

    rgb_led.Begin(); // Enable the LEDs, so it can be turned off and not left as a floating pin.
    LedRGBOFF(); // Turn off the LEDs.
    
    gpsSerial.begin(GPSBaud); // Enable the serial connection to the GPS module

    os_init(); // LMIC init
    LMIC_reset(); // Reset the MAC state. Session and pending data transfers will be discarded.
    //LMIC_setClockError(MAX_CLOCK_ERROR * 10 / 100); //Recommended on github to add this //Testing if this caused failure //Not needed, but also don't seem to cause major issues.

    // Set static session parameters. Instead of dynamically establishing a session
    // by joining the network, precomputed session parameters are be provided.
    #ifdef PROGMEM
        // On AVR, these values are stored in flash and only copied to RAM
        // once. Copy them to a temporary buffer here, LMIC_setSession will
        // copy them into a buffer of its own again.
        uint8_t appskey[sizeof(APPSKEY)];
        uint8_t nwkskey[sizeof(NWKSKEY)];
        memcpy_P(appskey, APPSKEY, sizeof(APPSKEY));
        memcpy_P(nwkskey, NWKSKEY, sizeof(NWKSKEY));
        LMIC_setSession (0x1, DEVADDR, nwkskey, appskey);
    #else
        // If not running an AVR with PROGMEM, just use the arrays directly
        LMIC_setSession (0x1, DEVADDR, NWKSKEY, APPSKEY);
    #endif

    LMIC_selectSubBand(1); //CFG_us915 https://github.com/TheThingsNetwork/gateway-conf/blob/master/US-global_conf.json
    LMIC_setLinkCheckMode(0); // Disable link check validation
    LMIC.dn2Dr = DR_SF9; // TTN uses SF9 for its RX2 window.
    LMIC_setDrTxpow(DR_SF9,27); // Set data rate and transmit power for uplink (note: txpow seems to be ignored by the library, although 27 is the max according to examples/raw/raw.ino)
   
    // Start job
    do_send(&sendjob); //Start job of getting GPS data and sending
}

void loop() {
    os_runloop_once();
}