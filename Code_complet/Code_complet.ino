#include <Adafruit_SSD1306.h>

// Parameter definition
/////////////////////////////////////////////////////////////////////////
#define encoderPinA 2 // CLK Output A
#define encoderPinB 4 // DT Output B
#define Switch 5 // Switch connection

volatile int lastEncoded = 0;
volatile long encoderValue = 0;
long lastEncoderValue = 0;

long lastDebounceTime = 0;
long debounceDelay = 120;

#define nombreDePixelsEnLargeur 128
#define nombreDePixelsEnHauteur 64
#define brocheResetOLED -1
#define adresseI2CecranOLED 0x3C

Adafruit_SSD1306 ecranOLED(nombreDePixelsEnLargeur, nombreDePixelsEnHauteur, &Wire, brocheResetOLED);

String menuItems[] = {"Potentiometer", "Flex Sensor", "Graphite Sensor"}; // Definition of menu items
int selectedItem = 0; // Menu items are defined from 0 to 2
/////////////////////////////////////////////////////////////////////////

void setup() {
  Serial.begin(9600);
  pinMode(encoderPinA, INPUT_PULLUP); // Use internal pull-up resistors for the encoder
  pinMode(encoderPinB, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(encoderPinB), updateEncoder, RISING); // Whenever the encoderPinB changes state, an interruption occurs.
  
  if (!ecranOLED.begin(SSD1306_SWITCHCAPVCC, adresseI2CecranOLED))
    while (1); // Program stops (infinite loop) if something goes wrong here

  ecranOLED.clearDisplay();
  ecranOLED.setTextColor(SSD1306_WHITE);
}

void loop() {
  displayMenu();
  encoderValue = abs(encoderValue % 3); 
  Serial.println(encoderValue); // To debug
  if (encoderValue != lastEncoderValue) { // If we turn the encoder, then the selectedItem variable is increased
    selectedItem++;
    if (selectedItem == 3) {
      selectedItem = 1; // If we reach the last element from the menu, the selectedItem is reset to the first element
    }
  }  
  Serial.println("Selected:" + String(selectedItem)); // To debug
  delay(150); // We add a delay between loops for stability, while still having a responsive display
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
  switch (selectedItem) {
    case 1:
    
      break;
    case 2:
      // Action for Item 2
      break;
    case 3:
      // Action for Item 3
      break;
    default:
      break;
  }
}

void updateEncoder() {
  if (encoderPinB == HIGH) {
    encoderValue++;
  }
  else {
    encoderValue--;
  }
}