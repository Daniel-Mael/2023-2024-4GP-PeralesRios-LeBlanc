/*
Complete Arduino code for "Projet capteur".

Students: Maël LE BLANC, Daniel PERALES RIOS

Year / Department: 4th Year / Génie Physique

Description of the project:
    This code reads the values from a flex sensor and a graphite sensor, and allows to adjust the value of a digital potentiometer. This
    data is then displayed on an OLED screen, which is controlled thanks to a rotary encoder that has also a button. This rotary encoder
    allows to choose the items in the menu and to enter the selected item by pressing the switch button in the encoder. All the acquired 
    data is then sent via Bluetooth to an HC-05 Bluetooth module, that then sends this data to a remote device, such a smartphone. To allow
    communication between the Bluetooth module and the smartphone, we have build an app using MIT's App Inventor site and then uploading
    the .aia file to the smartphone via a QR code.
*/

// LIBRARIES
#include <Adafruit_SSD1306.h>
#include <SoftwareSerial.h>
#include <SPI.h>

//--------------------------------------- PARAMETER DEFINITION ---------------------------------------------------

// PINS: defining their values and other parameters
#define encoderPinA 2                   // CLK Output A
#define encoderPinB 4                   // DT Output B
#define Switch 5                        // Switch connection
#define nombreDePixelsEnLargeur 128     // Parameters for the OLED screen
#define nombreDePixelsEnHauteur 64
#define brocheResetOLED -1
#define adresseI2CecranOLED 0x3C
#define MCP_NOP 0b00000000              // Parameters for the digital potentiometer
#define MCP_WRITE 0b00010001
#define MCP_SHTDWN 0b00100001
#define rxPin 11                        // Pin 11 as RX, connecto to TX in the HC-05
#define txPin 10                        // Pin 10 as TX, connecto to RX in the HC-05
#define baudrate 38400                  // Chosen baudrate for the Bluetooth, to avoid bugs

// OLED SCREEN AND BLUETOOTH MODULE: declaring the associated objects
Adafruit_SSD1306 ecranOLED(nombreDePixelsEnLargeur, nombreDePixelsEnHauteur, &Wire, brocheResetOLED);
SoftwareSerial mySerial(rxPin, txPin);

// FLEX SENSOR: constants and variables definition
const int flexPin = A1;                 // Pin connected to voltage divider output
const float VCC = 5.0;                  // Voltage at Ardunio 5V line
const float R_DIV = 56000.0;            // Resistor used to create a voltage divider
const float flatResistance = 31000.0;   // Resistance when flat
const float bendResistance = 67000.0;   // Resistance at 90 deg bending
float Rflex = 0; 
float angle = 0; 

// ROTARY ENCODER AND SWITCH BUTTON: variable definition
int switchState;                        // The current reading from the switch pin
int lastSwitchState = HIGH;             // The previous reading from the switch pin (switch is HIGH when we don't press)
long lastEncoderValue = 0;              // Used for the selection of the menu
long lastPotValue = 0;                  // Used for the digital potentiometer
long lastDebounceTime = 0;              // The last time the output pin was toggled
long debounceDelay = 40;                // The debounce time, increase if the output flickers
bool button_pressed = false;            // This variable becomes true whenever the switch button is pressed, false otherwise
volatile long encoderValue = 0;         // This variable will store the value of the rotary encoder for moving through the menus
volatile long encoder_for_pot = 0;      // This variable will store the value of the rotary encoder for selecting the potentiometer resistance

// MENU DISPLAY: constants and variables definition
bool MainMenu = true;                                                   // This bool is true when we are in the main menu, false otherwise. Useful to avoid bugs
const char* menuItems[] = {"Potentiometer", "Flex Sensor", 
                           "Graphite Sensor"};                          // Definition of menu items. We avoid using String, as they are memory-intensive, and we use a more C-style constant.
const char* potentiometerItems[] = {"DigiPot (50k) setting", 
                                    "Value R2:", "", "BACK"};           // Definition of items in the potentiometer screen
const char* flexItems[] = {"Flex sensor reading", "Resistance value:", 
                            "", "Bend angle:", "", "BACK"};             // Definition of items in the flex sensor screen
int selectMenu = 0;                                                     // Menu items are defined from 0 to x (depending on the menu)
int selectItem = 0;                                                     // Item selection in each menu


// DIGITAL POTENTIOMETER: constants and variables definition
const int ssMCPin = 10;                 // Define the slave select for the digital potentiometer
float rWiper = 125;
float max_value_pot = 50000;            // Maximum resistance for the digital potentiometer (we supposed here 100k)
int valuePot = 15;                      // valeur initiale envoyée au potentiomètre
float R2 = (10/256)*max_value_pot;      // We set up the value of the digital potentiometer at a default value

//----------------------------------------------------------------------------------------------------------------


void setup() {    
    // ROTARY ENCODER AND SWITCH BUTTON
    pinMode(encoderPinA, INPUT);        // Use internal pull-up resistors for the encoder
    digitalWrite(encoderPinA, HIGH);    // Turn on pull-up resistor
    pinMode(encoderPinB, INPUT);
    digitalWrite(encoderPinB, HIGH);    // Turn on pull-up resistor
    pinMode(Switch, INPUT);
    digitalWrite(Switch, HIGH);         // Turn on pull-up resistor

    // DIGITAL POTENTIOMETER
    pinMode(ssMCPin, OUTPUT);
    digitalWrite(ssMCPin, HIGH);        // SPI chip disabled
    SPI.begin();

    // FLEX SENSOR
    pinMode(flexPin, INPUT);

    // BLUETOOTH MODULE
    pinMode(rxPin, INPUT);
    pinMode(txPin, OUTPUT);
    mySerial.begin(baudrate); 

    // INTERRUPTION FOR THE ROTARY ENCODER
    attachInterrupt(0, updateEncoder, CHANGE);
    
    // OLED SCREEN SETUP
    if (!ecranOLED.begin(SSD1306_SWITCHCAPVCC, adresseI2CecranOLED))
        while (1); // If the screen doesn't start, the program stays blocked here forever        
    ecranOLED.clearDisplay();
    ecranOLED.setTextColor(SSD1306_WHITE);
    
    // SERIAL COMMUNICATION INITIALIZATION
    Serial.begin(baudrate);
}


void loop() {
    if (MainMenu) {
        displayMenu();
        updateSelectedItem(encoderValue, (sizeof(menuItems) / sizeof(menuItems[0])));
        switchButton();
    }
    else {
        handleMenuItemSelection(selectMenu);
        switchButton();
    }
    int ADCflex = analogRead(flexPin);                            // Flex sensor: read the ADC, and calculate voltage and resistance from it
    float Vflex = (ADCflex / 1024.0) * VCC;
    Rflex = R_DIV * (VCC / Vflex - 1.0);                          // Flex sensor: we calculate the approximate resistance value
    angle = map(Rflex, flatResistance, bendResistance, 0, 90.0);  // Flex sensor: use the calculated resistance to estimate the 
                                                                  // sensor's bend angle
    delay(200);                                                   // We add a delay between loops for stability, while still having a responsive display
}


void displayMenu() {
  ecranOLED.clearDisplay();
  for (int i = 0; i < sizeof(menuItems) / sizeof(menuItems[0]); i++) {
    if (i == selectMenu) {
      ecranOLED.setTextColor(SSD1306_BLACK, SSD1306_WHITE); // Highlight selected item
    } else {
      ecranOLED.setTextColor(SSD1306_WHITE);                // Regular color for other items
    }
    ecranOLED.setCursor(0, i * 10);                         // Adjust position for each item
    ecranOLED.print(menuItems[i]);                          // Print each element
  }
  ecranOLED.display();                                      // Display all the elements from the buffer
}


void handleMenuItemSelection(int selectMenu) {
  switch (selectMenu) {       // For each case, if the switch button was pressed, we enter each menu respectively
    case 0:                   // Potentiometer setting using a dedicated function 
      selectItem = 0;
      displayPotentiometer(valuePot);
      updateSelectedItem(encoderValue, sizeof(potentiometerItems) / sizeof(potentiometerItems[0])); 
      break;
    case 1:                   // Flex sensor reading and display using a dedicated function
      selectItem = 0;
      displayFlexSensor(); 
      break;
    case 2:                   // Graphite sensor reading and display using a dedicated function
      selectItem = 0;
      displayGraphiteSensor();
      break;
    default:                  // In case a bug occurs with selectMenu, the code breaks here to avoid extra bugs
      break;
  }
}


void updateEncoder() {
    /*
    This function allows to control the menu when the rotary encoder is turned. This function takes as input the 
    number of elements of each menu to modify the last element than can be selected accordingly.
    */

    if (MainMenu){
      if (digitalRead(encoderPinB) == HIGH) {
          encoderValue++;
      }
      else {
          encoderValue--;
      }
    }
    else if (selectMenu == 0) {
      if (digitalRead(encoderPinB) == HIGH) {
          encoder_for_pot++;
      }
      else {
          encoder_for_pot--;
      }
    }

}

void updatePotentiometerValue(){
  if (encoder_for_pot != lastPotValue) {  // If we turn the encoder, then the selectedItem variable is increased
      valuePot = valuePot + 10;           // valuePot goes from 0 to 255. 255 corresponds to 50 kohms
      if (valuePot > 255){
        valuePot=0;                       // If valuePot is bigger than 255, we reset its value to 0
      }
  }
  lastPotValue = encoder_for_pot;         // The last value from the encoder is updated
  valuePot = constrain(valuePot, 0, 255);
}


void updateSelectedItem(int encoderValue, int NumberOfItems) {
  encoderValue = abs(encoderValue % NumberOfItems); 
  if (encoderValue != lastEncoderValue) {     // If we turn the encoder, then the selectedItem variable is increased
      selectMenu++;
      if (selectMenu == NumberOfItems) {
          selectMenu = 0;                     // If we reach the last element from the menu, the selectedItem is reset to the first element
      }  
  }
  lastEncoderValue = encoderValue;            // The last value from the encoder is updated
}


void SPIWrite(uint8_t cmd, uint8_t data, uint8_t ssPin) {
  /*
  This function controls the communication between the Arduino and the digital potentiometer
  */

  // https://www.arduino.cc/en/Reference/SPISettings
  updatePotentiometerValue();
  R2 = ((max_value_pot * data) / 256 ) + rWiper;
  SPI.beginTransaction(SPISettings(14000000, MSBFIRST, SPI_MODE0));
  digitalWrite(ssPin, LOW);   // SS pin low to select chip
  SPI.transfer(cmd);          // Send command code
  SPI.transfer(data);         // Send associated value
  digitalWrite(ssPin, HIGH);  // SS pin high to de-select chip
  SPI.endTransaction();
}


void switchButton() {
  /*
  This function controls the reading of the switch button. It has debouncing logic to minimize bad readings.
  */

  // Check if the switch button is pressed and enough time has passed since the last press
  if (digitalRead(Switch) == LOW && (millis() - lastDebounceTime) >= debounceDelay) {
    // Record the time when the button was pressed
    lastDebounceTime = millis();

    // Set the button_pressed flag only if it's not already set
    if (!button_pressed) {
      button_pressed = true;

      // If we were in the main menu, switch to the menu item view
      if (MainMenu) {
        MainMenu = false;
        selectItem = 0;
      } else {
        // If we were in a menu item view, switch back to the main menu
        MainMenu = true;
      }
    }
  } else {
    // Reset button_pressed if the button is released
    button_pressed = false;
  }
}


void sendBluetooth(float potValue, float flexValue, float graphiteValue) {
  /*
  This function sends all the gathered data to the Bluetooth module, that transfers the data to a mobile phone through an Android app
  */

  uint8_t data_pot = potValue; 
  uint8_t data_flex = flexValue; 
  uint8_t data_graphite = graphiteValue; 
  mySerial.write(data_pot);       // Sending the data through Bluetooth
  delay(50);                      // Introducing a small delay for data transfer + stability
  mySerial.write(data_flex);      
  delay(50);                      
  mySerial.write(data_graphite);  
  delay(50);                     
}


void displayPotentiometer(int valuePot) {
    /*
    Dedicated funciton for the menu corresponding to the digital potentiometer and its setting.
    */

    ecranOLED.clearDisplay();
    SPIWrite(MCP_WRITE, valuePot, ssMCPin);
    dtostrf(R2, 6, 2, potentiometerItems[2]);         // Update the value of the potentiometer resistance, which is set at the beginning at a default value
    for (int i = 0; i < sizeof(potentiometerItems) / sizeof(potentiometerItems[0]); i++) {
        ecranOLED.setTextColor(SSD1306_WHITE);        // Regular color for other items
        ecranOLED.setCursor(0, i * 10);               // Adjust position for each item
        ecranOLED.print(potentiometerItems[i]);       // Print each element
    }
    switchButton();
    ecranOLED.display();
}


void displayFlexSensor() {
    ecranOLED.clearDisplay();
    dtostrf(Rflex, 6, 2, flexItems[2]);         // Update the values of the resistance and the angle of the flex sensor
    dtostrf(angle, 6, 2, flexItems[4]);
    for (int i = 0; i < sizeof(flexItems) / sizeof(flexItems[0]); i++) {
        ecranOLED.setTextColor(SSD1306_WHITE);  // Regular color for other items
        ecranOLED.setCursor(0, i * 10);         // Adjust position for each item
        ecranOLED.print(flexItems[i]);          // Print each element
    }
    switchButton();
    ecranOLED.display();
}


void displayGraphiteSensor() {
    ecranOLED.clearDisplay();
    ecranOLED.setTextColor(SSD1306_WHITE);
    ecranOLED.setCursor(0, 0);
    ecranOLED.print("Graphite sensor: " + String(5000) + "ohms");     
    switchButton();
    ecranOLED.display();
}
