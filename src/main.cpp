#define ARDUINOJSON_ENABLE_STD_STRING 0
#define ARDUINOJSON_ENABLE_STD_STREAM 0
#include <iostream>
#include <Arduino.h>
#include <SPI.h>
#include <BLEPeripheral.h>
#include <array>


//custom boards may override default pin definitions with BLEPeripheral(PIN_REQ, PIN_RDY, PIN_RST)
BLEPeripheral blePeripheral = BLEPeripheral();

// create service
BLEService bleService = BLEService("19b10001e8f2537e4f6cd104768a1215");

// create switch characteristic
BLECharCharacteristic bleCharacteristic = BLECharCharacteristic("19b10001e8f2537e4f6cd104768a1215", BLERead | BLEWrite);

bool waitingForValue = false;
uint8_t previousCommand;
char config[4] ={0,0,0,0}; 
float val_adj;

void blePeripheralConnectHandler(BLECentral& central);
void blePeripheralDisconnectHandler(BLECentral& central);
void switchCharacteristicWritten(BLECentral& central, BLECharacteristic& characteristic);

String generateRandomString() {
  String randomString;

  // Generate 4 random digits
  for (int i = 0; i < 4; ++i) {
    randomString += String(random(10));
  }

  // Generate 3 random letters
  for (int i = 0; i < 3; ++i) {
    randomString += char(random('A', 'Z' + 1));
  }

  return randomString;
}

void setup() {
  Serial.begin(9600);

  // set advertised local name and service UUID
  blePeripheral.setLocalName("Smart Garage System");
  blePeripheral.setAdvertisedServiceUuid(bleService.uuid());

  // add service and characteristic
  blePeripheral.addAttribute(bleService);
  blePeripheral.addAttribute(bleCharacteristic);

  // assign event handlers for connected, disconnected to peripheral
  blePeripheral.setEventHandler(BLEConnected, blePeripheralConnectHandler);
  blePeripheral.setEventHandler(BLEDisconnected, blePeripheralDisconnectHandler);

  // assign event handlers for characteristic
  bleCharacteristic.setEventHandler(BLEWritten, switchCharacteristicWritten);

  // begin initialization
  blePeripheral.begin();

  Serial.println(F("BLE LED Peripheral"));
}

void loop() {
  // poll peripheral
  blePeripheral.poll();

  if (!(millis() % 5000)) {

    String randomString = generateRandomString();
    
    // Convert the String to a char array for BLE transmission
    char randomCharArray[randomString.length() + 1];
    randomString.toCharArray(randomCharArray, randomString.length() + 1);

    // Set the value of the BLE characteristic
    bleCharacteristic.setValue(*randomCharArray);
    
    Serial.println("Sent Random String: " + randomString);
  }
}


void blePeripheralConnectHandler(BLECentral& central) {
  // central connected event handler
  Serial.print(F("Connected event, central: "));
  Serial.println(central.address());
  
  //std::cout << "Configuració al inicialitzar: ";
  //for (int value : configuration) {
    //  std::cout << value << " ";
  //}

}

void blePeripheralDisconnectHandler(BLECentral& central) {
  // central disconnected event handler
  Serial.print(F("Disconnected event, central: "));
  Serial.println(central.address());
}

void switchCharacteristicWritten(BLECentral& central, BLECharacteristic& characteristic) {
  // central wrote new value to characteristic
  if (!waitingForValue) {
    Serial.print(F("Comanda rebuda: "));

    uint8_t command = previousCommand = bleCharacteristic.value();
    Serial.println(command);

    switch (command) {
      case 0x01:
        // Habilitar càmera
        Serial.println("Habilitar càmera");
        config[0] = 128;
        break;
      
      case 0x02:
        //Deshabilitar càmera
        Serial.println("Deshabilitar càmera");
        config[0] = 0;
        break;
      
      case 0x03:
        // Habilitar sensor llum
        Serial.println("Habilitar sensor llum");
        config[1] = 128;
        break;

      case 0x04:
        // Deshabilitar sensor llum
        Serial.println("Deshabilitar sensor llum");
        config[1] = 0;
        break;
      
      case 0x05:
        // Sensibilitat càmera
        waitingForValue = true;
        break;
      
      case 0x06:
        // Sensibilitat sensor de llum
        waitingForValue = true;
        break;
      
      case 0x07:
        // error temporal permès
        waitingForValue = true;
        break;

      case 0x08:
        // Canviar zona horària
        waitingForValue = true;
        return;
      
      default:
        Serial.println("Comanda invàlida");
    }
  } else {
    uint8_t value = bleCharacteristic.value();

    switch (previousCommand) {

      case 0x05:
        // Sensibilitat càmera
        Serial.print("Sensibilitat càmera: ");
        val_adj = value/255.0;
        Serial.println(val_adj);
        config[0] = value;
        waitingForValue = false;
        break;

      case 0x06:
        // Sensibilitat sensor de llum
        Serial.print("Sensibilitat sensor de llum: ");
        val_adj = value/255.0;
        Serial.println(val_adj);
        config[1] = value;
        waitingForValue = false;
        break;

      case 0x07:
        // Error temporal
        Serial.print("Error permès en segons: ");
        val_adj = value/255.0;
        Serial.println(val_adj);
        config[2] = value;
        waitingForValue = false;
        break;
      
      case 0x08:
        // Canviar zona horària
        Serial.print("Valor zona horària: ");
        config[3] = value;
        Serial.println(value);
        break;
      default:
        break;
    }

    waitingForValue = false;
  }
}
