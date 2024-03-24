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

// Definition of the libraries used in the code
#include <Adafruit_SSD1306.h>
#include <SPI.h>

// Parameter definition
/////////////////////////////////////////////////////////////////////////

// We define the value of the pins
#define encoderPinA 2 // CLK Output A
#define encoderPinB 4 // DT Output B
#define Switch 5 // Switch connection
#define nombreDePixelsEnLargeur 128 // Parameters for the OLED screen
#define nombreDePixelsEnHauteur 64
#define brocheResetOLED -1
#define adresseI2CecranOLED 0x3C

Adafruit_SSD1306 ecranOLED(nombreDePixelsEnLargeur, nombreDePixelsEnHauteur, &Wire, brocheResetOLED);

// Constants and variables definition
const int flexPin = A1;      // pin connected to voltage divider output
const float VCC = 5.0;      // voltage at Ardunio 5V line
const float R_DIV = 56000.0;  // resistor used to create a voltage divider
const float flatResistance = 31000.0; // resistance when flat
const float bendResistance = 67000.0;  // resistance at 90 deg bending
volatile long encoderValue = 0;
long lastEncoderValue = 0;
long lastDebounceTime = 0;
long debounceDelay = 120;
long valuePot = 0; // Potentiometer value; 
bool action = false; // This variable becomes true whenever the switch button is pressed, false otherwise
bool MainMenu = true; // This bool is true when we are in the main menu, false otherwise. Useful to avoid bugs
String menuItems[] = {"Potentiometer", "Flex Sensor", "Graphite Sensor"}; // Definition of menu items
String potentiometerItems[] = {"Potentiometer reading", "Current value:", "BACK"}; // Definition of items in the potentiometer screen
String flexItems[] = {"Flex sensor reading", "Resistance value:", "", "Bend angle:", "", "BACK"}; // Definition of items in the flex sensor screen
int selectedItem = 0; // Menu items are defined from 0 to x (depending on the menu)
float Rflex = 0; 
float angle = 0; 

/////////////////////////////////////////////////////////////////////////



void setup() {
    Serial.begin(9600);
    pinMode(encoderPinA, INPUT_PULLUP); // Use internal pull-up resistors for the encoder
    pinMode(encoderPinB, INPUT_PULLUP);
    attachInterrupt(digitalPinToInterrupt(encoderPinB), updateEncoder, RISING); // Whenever the encoderPinB changes state, an interruption occurs.
    attachInterrupt(digitalPinToInterrupt(Switch), switchButton, RISING); // Whenever the switch button is pressed, an interruption is raised
    
    if (!ecranOLED.begin(SSD1306_SWITCHCAPVCC, adresseI2CecranOLED))
        while (1); // Program stops (infinite loop) if something goes wrong here
        
    pinMode(flexPin, INPUT);
    ecranOLED.clearDisplay();
    ecranOLED.setTextColor(SSD1306_WHITE);
}



void loop() {
    if (MainMenu) {
        displayMenu();
        updateEncoder((sizeof(menuItems) / sizeof(menuItems[0])));  // The number of items of a list is defined as the number of bytes of the
                                                                    // whole list divided by the bytes occupied by one element
    }
    else {
        handleMenuItemSelection(selectedItem);
    }
    int ADCflex = analogRead(flexPin); // Flex sensor: read the ADC, and calculate voltage and resistance from it
    float Vflex = (ADCflex / 1024.0) * VCC;
    Rflex = R_DIV * (VCC / Vflex - 1.0); // Flex sensor: we calculate the approximate resistance value
    angle = map(Rflex, flatResistance, bendResistance, 0, 90.0);  // Flex sensor: use the calculated resistance to estimate the 
                                                                        // sensor's bend angle
    delay(200); // We add a delay between loops for stability, while still having a responsive display
}



void displayMenu() {
  ecranOLED.clearDisplay();
  for (int i = 0; i < sizeof(menuItems) / sizeof(menuItems[0]); i++) {
    if (i == selectedItem) {
      ecranOLED.setTextColor(SSD1306_BLACK, SSD1306_WHITE); // Highlight selected item
    } else {
      ecranOLED.setTextColor(SSD1306_WHITE); // Regular color for other items
    }
    ecranOLED.setCursor(0, i * 10); // Adjust position for each item
    ecranOLED.print(menuItems[i]); // Print each element
  }
  ecranOLED.display();
}



void handleMenuItemSelection(int selectedItem) {
  // Implement actions for each menu item
  switch (selectedItem) { // For each case, if the switch button was pressed, we enter each menu respectively
    case 0: // Potentiometer setting using a dedicated function
      if (action) {
        displayPotentiometer(valuePot);
        updateEncoder(sizeof(potentiometerItems) / sizeof(potentiometerItems[0])); 
      }
    case 1: // Flex sensor reading and display using a dedicated function
      if (action) {
        displayFlexSensor(valueFlex); 
        updateEncoder(sizeof(flexItems) / sizeof(flexItems[0])); 
      }
    case 2: // Graphite sensor reading and display using a dedicated function
      if (action) {
        
      }
    default:
      break;
  }
}



void updateEncoder(int NumberOfItems) {
    /*
    This function allows to control the menu when the rotary encoder is turned. This function takes as input the 
    number of elements of each menu to modify the last element than can be selected accordingly.
    */

    if (encoderPinB == HIGH) {
        encoderValue++;
    }
    else {
        encoderValue--;
    }
    encoderValue = abs(encoderValue % 3); 
    Serial.println(encoderValue); // To debug
    if (encoderValue != lastEncoderValue) { // If we turn the encoder, then the selectedItem variable is increased
        selectedItem++;
        if (selectedItem == NumberOfItems) {
            selectedItem = 0; // If we reach the last element from the menu, the selectedItem is reset to the first element
        }  
        Serial.println("Selected:" + String(selectedItem)); // To debug
    }
    lastEncoderValue = encoderValue; // The last value from the encoder is updated
}



void switchButton() {
  /*
  This function controls the reading of the switch button. It has debouncing logic to minimize bad readings.
  */

  long currentTime = millis();
  lastDebounceTime = currentTime;
  while (currentTime - lastDebounceTime < debounceDelay) {
    currentTime = millis(); // The function waits for the debounce delay to pass, and then we read the value of the switch again
  }
  if (Switch == HIGH) { // If, after the debounce delay, the Switch is still high, we have effectively pressed the button
    action = true; // We choose the selected option
    if (MainMenu) {
        MainMenu = false; // If we were in the main menu, we set this variable to false so then the option and not the menu is displayed
        selectedItem = 0; // We reset selectedItem to 0 so we can control the different menus with this same variable
    }
  }
}



void displayPotentiometer(int valuePot) {
    /*
    Dedicated funciton for the menu corresponding to the digital potentiometer and its setting.
    */

    ecranOLED.clearDisplay();
    for (int i = 0; i < sizeof(potentiometerItems) / sizeof(potentiometerItems[0]); i++) {
        ecranOLED.setTextColor(SSD1306_WHITE); // Regular color for the items
        ecranOLED.setCursor(0, i * 10); // Adjust position for each item
        ecranOLED.print(potentiometerItems[i]); // Print each element
    }
    if (selectedItem != 0) {
        ecranOLED.setCursor(0, ((sizeof(potentiometerItems) / sizeof(potentiometerItems[0])) - 1) * 10); // We set the cursor in the last element (back button)
        ecranOLED.setTextColor(SSD1306_BLACK, SSD1306_WHITE); // Highlight the back button
    }

    ecranOLED.display();
}



void displayFlexSensor(int valueFlex) {
    ecranOLED.clearDisplay();
    flexItems[2] = Rflex; // Update the values of the resistance and the angle of the flex sensor
    flexItems[4] = angle; 
    for (int i = 0; i < sizeof(flexItems) / sizeof(flexItems[0]); i++) {
        ecranOLED.setTextColor(SSD1306_WHITE); // Regular color for other items
        ecranOLED.setCursor(0, i * 10); // Adjust position for each item
        ecranOLED.print(flexItems[i]); // Print each element
    }
    ecranOLED.display();
}

