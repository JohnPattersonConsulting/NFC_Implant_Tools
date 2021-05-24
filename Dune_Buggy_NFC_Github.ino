// NFC ignition interlock program for the POWER BUGGY control system
// 2021 John Patterson Consulting, LLC
// Based on the Adafruit PN532 library and its examples.

#include <Wire.h>
#include <SPI.h>
#include <Adafruit_PN532.h>

// Define the pins for SPI communication:
#define PN532_SCK  (2)
#define PN532_MOSI (3)
#define PN532_SS   (4)
#define PN532_MISO (5)

// Initialize PN532 object on software SPI conection called "nfc":
Adafruit_PN532 nfc(PN532_SCK, PN532_MISO, PN532_MOSI, PN532_SS);

// NDEF and Mifare Classic keys:
uint8_t keyuniversal[6] = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };

// Variables for unlocking the ignition:
bool unlocked = 0;
bool unlocked2 = 0;

// Pin numbers for LEDs and relays:
const int relay = 6;
const int redLED = 7;
const int greenLED = 13;

// Secret key (this is the password written to NDEF):
uint8_t key[4] = {1, 1, 1, 1};

// Location page of secret key (this is where the password is written to NDEF):
int pageLocation2 = 100;
int pageLocation1 = 10;

// Setup function:
void setup(void) {
  // Turn on serial port for troubleshooting purposes:
  Serial.begin(115200);
  Serial.println("Hello!");

  // Begin connnection to PN532 board on object "nfc":
  nfc.begin();

  // Check for PN532 board and check firmware version:
  uint32_t versiondata = nfc.getFirmwareVersion();
  if (!versiondata) 
  {
    Serial.print("Didn't find PN53x board");
    while (1); // halt
  }
  
  // If data is available, report the board type:
  Serial.print("Found chip PN5"); Serial.println((versiondata>>24) & 0xFF, HEX); 
  Serial.print("Firmware ver. "); Serial.print((versiondata>>16) & 0xFF, DEC); 
  Serial.print('.'); Serial.println((versiondata>>8) & 0xFF, DEC);
  
  // Configure board to read RFID tags:
  nfc.SAMConfig();

  // Indicate that system is waiting for an NFC tag:
  Serial.println("Waiting for an ISO14443A Card ...");

  // Set pin modes for indication LEDs and relay:
  pinMode(relay, OUTPUT);
  pinMode(redLED, OUTPUT);
  pinMode(greenLED, OUTPUT);

  // Turn on "red" LED:
  digitalWrite(redLED, HIGH);
}

void loop(void) {
  uint8_t success;
  uint8_t uid[] = { 0, 0, 0, 0, 0, 0, 0 };  // Buffer to store the returned UID
  uint8_t uidLength;                        // Length of the UID (4 or 7 bytes depending on ISO14443A card type)
    
  // Wait for an NTAG203 card.  When one is found 'uid' will be populated with
  // the UID, and uidLength will indicate the size of the UUID (normally 7)
  success = nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, uid, &uidLength);
  
  if (success) {
    // Display some basic information about the card
    Serial.println("Found an ISO14443A card");
    Serial.print("  UID Length: ");Serial.print(uidLength, DEC);Serial.println(" bytes");
    Serial.print("  UID Value: ");
    nfc.PrintHex(uid, uidLength);
    Serial.println("");
    
    if (1)
    {
      uint8_t data[32]  = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
            
        success = nfc.mifareclassic_AuthenticateBlock (uid, uidLength, pageLocation1, 1, keyuniversal);
        success = nfc.mifareclassic_ReadDataBlock(10, data);

        // Display the results, depending on 'success'
        if (success) 
        {
          // Dump the page data
          unlocked = 1;
          for(int i = 0; i < 4; i++)
          {
            Serial.println(data[i]);
            if(data[i] != key[i])
            {
              unlocked = 0;
            }
          }
          Serial.println(unlocked);
        }
        else
        {
          Serial.println("Unable to read the requested page at location 1!");
        }    

        nfc.ntag2xx_ReadPage(pageLocation2, data);

        // Display the results, depending on 'success'
        if (success) 
        {
          // Dump the page data
          unlocked2 = 1;
          for(int i = 0; i < 4; i++)
          {
            Serial.println(data[i]);
            if(data[i] != key[i])
            {
              unlocked2 = 0;
            }
          }
          Serial.println(unlocked2);

        }
        else
        {
          Serial.println("Unable to read the requested page at location 2!");
        }    


        if(unlocked2 || unlocked)
          {
            digitalWrite(relay, HIGH);
            digitalWrite(redLED, LOW);
            digitalWrite(greenLED, HIGH);
          }
          
    }
    else
    {
      Serial.println("This doesn't seem to be an NTAG203 tag (UUID length != 7 bytes)!");
    }
  }

}
