
#include <SPI.h>
#include <LoRa.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

//====================================================
// LORA PIN CONFIGURATION - ARDUINO UNO
//====================================================

#define LORA_SS       10
#define LORA_RST       9
#define LORA_DIO0      2

#define LORA_FREQUENCY 433E6

//====================================================
// LCD CONFIGURATION
//====================================================

LiquidCrystal_I2C lcd(0x27, 16, 2);

//====================================================
// VARIABLES
//====================================================

char receivedData[32];

int temperature = 0;
int humidity = 0;

unsigned long packetCount = 0;

//====================================================
// SETUP
//====================================================

void setup()
{
    Serial.begin(9600);

    delay(1000);

    Serial.println();
    Serial.println("================================");
    Serial.println(" ARDUINO UNO LORA RECEIVER");
    Serial.println("================================");

    //================================================
    // LCD INITIALIZATION
    //================================================

    lcd.init();
    lcd.backlight();
    lcd.clear();

    lcd.setCursor(0, 0);
    lcd.print("LORA RECEIVER");

    lcd.setCursor(0, 1);
    lcd.print("INITIALIZING...");

    //================================================
    // LORA INITIALIZATION
    //================================================

    LoRa.setPins(
        LORA_SS,
        LORA_RST,
        LORA_DIO0
    );

    Serial.println("INITIALIZING LORA...");

    if (!LoRa.begin(LORA_FREQUENCY))
    {
        Serial.println("LORA INIT FAILED!");

        lcd.clear();
        lcd.print("LORA INIT FAILED");

        while (1)
        {
            delay(1000);
        }
    }

    //================================================
    // SAME CONFIGURATION AS ESP32 TRANSMITTER
    //================================================

    LoRa.setSpreadingFactor(7);

    LoRa.setSignalBandwidth(125E3);

    LoRa.setCodingRate4(5);

    LoRa.setSyncWord(0x12);

    LoRa.enableCrc();

    //================================================
    // INITIALIZATION SUCCESS
    //================================================

    Serial.println("LORA INITIALIZATION SUCCESS");

    Serial.println("--------------------------------");
    Serial.println("FREQUENCY : 433 MHz");
    Serial.println("SF        : 7");
    Serial.println("BANDWIDTH : 125 kHz");
    Serial.println("CODING    : 4/5");
    Serial.println("SYNC WORD : 0x12");
    Serial.println("CRC       : ENABLED");
    Serial.println("--------------------------------");

    Serial.println("WAITING FOR TEMPERATURE DATA...");

    lcd.clear();

    lcd.setCursor(0, 0);
    lcd.print("LORA READY");

    lcd.setCursor(0, 1);
    lcd.print("WAITING DATA...");
}

//====================================================
// LORA RECEIVE FUNCTION
//====================================================

void LoRa_Receive()
{
    int packetSize = LoRa.parsePacket();

    if (packetSize <= 0)
    {
        return;
    }

    // Reject oversized packets
    if (packetSize >= sizeof(receivedData))
    {
        while (LoRa.available())
        {
            LoRa.read();
        }

        Serial.println("PACKET TOO LARGE");
        return;
    }

    int dataLength = 0;

    // Read received packet
    while (LoRa.available() && dataLength < 31)
    {
        int value = LoRa.read();

        if (value >= 0)
        {
            receivedData[dataLength++] = (char)value;
        }
    }

    receivedData[dataLength] = '\0';

    // Check packet length
    if (dataLength != packetSize)
    {
        Serial.println("PACKET LENGTH ERROR");
        return;
    }

    //================================================
    // EXTRACT TEMPERATURE AND HUMIDITY
    // Expected format: T:30,H:70
    //================================================

    int t;
    int h;
    char extra;

    if (sscanf(receivedData, "T:%d,H:%d%c", &t, &h, &extra) != 2)
    {
        Serial.print("INVALID DATA: ");
        Serial.println(receivedData);
        return;
    }

    // Validate sensor values
    if (t < 0 || t > 50 || h < 0 || h > 100)
    {
        Serial.println("INVALID SENSOR VALUE");
        return;
    }

    temperature = t;
    humidity = h;

    packetCount++;

    //================================================
    // SERIAL MONITOR OUTPUT
    //================================================

    Serial.println();
    Serial.println("================================");

    Serial.print("PACKET COUNT : ");
    Serial.println(packetCount);

    Serial.print("RECEIVED DATA : ");
    Serial.println(receivedData);

    Serial.print("TEMPERATURE : ");
    Serial.print(temperature);
    Serial.println(" C");

    Serial.print("HUMIDITY : ");
    Serial.print(humidity);
    Serial.println(" %");

    Serial.print("RSSI : ");
    Serial.println(LoRa.packetRssi());

    Serial.print("SNR : ");
    Serial.println(LoRa.packetSnr());

    Serial.println("================================");

    //================================================
    // LCD DISPLAY
    //================================================

    lcd.setCursor(0, 0);

    lcd.print("Temp: ");

    lcd.print(temperature);

    lcd.print(" C     ");

    lcd.setCursor(0, 1);

    lcd.print("Humidity: ");

    lcd.print(humidity);

    lcd.print("%    ");
}

//====================================================
// MAIN LOOP
//====================================================

void loop()
{
    LoRa_Receive();
}
