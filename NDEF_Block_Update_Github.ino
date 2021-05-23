// NFC NDEF block writer for security interlock systems such as the POWER BUGGY NFC system.
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

// Secret passcode to write to the NFC card:
uint8_t page_to_write[4] = {1, 1, 1, 1};

// Location page in NDEF records to place the secret password:
int page_location = 99;

// Setup function:
void setup(void) {
  // Turn on serial port for troubleshooting purposes:
  Serial.begin(115200);
  Serial.println("Hello!");

  // Begin connnection to PN532 board on object "nfc":
  nfc.begin();

  // Check for PN532 board and check firmware version:
  uint32_t versiondata = nfc.getFirmwareVersion();
  if (! versiondata) {
    Serial.print("Didn't find PN53x board");
    while (1); // halt
  }
  
  // If data is available, report the board type:
  Serial.print("Found chip PN5"); Serial.println((versiondata>>24) & 0xFF, HEX); 
  Serial.print("Firmware ver. "); Serial.print((versiondata>>16) & 0xFF, DEC); 
  Serial.print('.'); Serial.println((versiondata>>8) & 0xFF, DEC);
  
  // Configure board to read RFID tags:
  nfc.SAMConfig();  
}

void loop(void) 
{
  uint8_t success;
  uint8_t uid[] = { 0, 0, 0, 0, 0, 0, 0 };  // Buffer to store the returned UID
  uint8_t uidLength;                        // Length of the UID (4 or 7 bytes depending on ISO14443A card type)
  uint8_t dataLength;

  // Require some user feedback before running this example! 
  // Otherwise, cards can be corrupted by partial engagement.
  Serial.println("\r\nWarning: Ensure good coupling between programmer and chip to avoid corruption!");
  Serial.println("\r\nPlace your NDEF formatted NTAG2xx tag on the reader to update the");
  Serial.println("NDEF record and press any key to continue ...\r\n");
  // Wait for user input before proceeding
  while (!Serial.available());
  // a key was pressed
  while (Serial.available()) Serial.read();

  // 1.) Wait for an NTAG203 card.  When one is found 'uid' will be populated with
  // the UID, and uidLength will indicate the size of the UID (normally 7)
  success = nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, uid, &uidLength);
  
  // It seems we found a valid ISO14443A Tag!
  if (success) 
  {
    // 2.) Display some basic information about the card
    Serial.println("Found an ISO14443A card");
    Serial.print("  UID Length: ");Serial.print(uidLength, DEC);Serial.println(" bytes");
    Serial.print("  UID Value: ");
    nfc.PrintHex(uid, uidLength);
    Serial.println("");
    
    if (uidLength != 7)
    {
      Serial.println("This doesn't seem to be an NTAG203 tag (UUID length != 7 bytes)!");
    }
    else
    {
      uint8_t data[32];
      
      // We probably have an NTAG2xx card (though it could be Ultralight as well)
      Serial.println("Seems to be an NTAG2xx tag (7 byte UID)");	  
      
      // 3.) Check if the NDEF Capability Container (CC) bits are already set
      // in OTP memory (page 3)
      memset(data, 0, 4);
      success = nfc.ntag2xx_ReadPage(3, data);
      if (!success)
      {
        Serial.println("Unable to read the Capability Container (page 3)");
        return;
      }
      else
      {
        // If the tag has already been formatted as NDEF, byte 0 should be:
        // Byte 0 = Magic Number (0xE1)
        // Byte 1 = NDEF Version (Should be 0x10)
        // Byte 2 = Data Area Size (value * 8 bytes)
        // Byte 3 = Read/Write Access (0x00 for full read and write)
        if (!((data[0] == 0xE1) && (data[1] == 0x10)))
        {
          Serial.println("This doesn't seem to be an NDEF formatted tag.");
          Serial.println("Page 3 should start with 0xE1 0x10.");
        }
        else
        {
          // 4.) Determine and display the data area size
          dataLength = data[2]*8;
          Serial.print("Tag is NDEF formatted. Data area size = ");
          Serial.print(dataLength);
          Serial.println(" bytes");

          // 5.) Try to add a new NDEF single page
          Serial.print("Writing page as NDEF Record ... ");
          success = nfc.ntag2xx_WritePage(page_location, page_to_write);
          if (success)
          {
            Serial.println("DONE!");
          }
          else
          {
            Serial.println("ERROR!");
          }
                    
        } // CC contents NDEF record check
      } // CC page read check
    } // UUID length check
    
    // Wait a bit before trying again
    Serial.flush();
    while (!Serial.available());
    while (Serial.available()) {
    Serial.read();
    }
    Serial.flush();    
  } // Start waiting for a new ISO14443A tag
}
