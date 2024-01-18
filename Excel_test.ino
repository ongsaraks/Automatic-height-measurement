#include <SPI.h>
#include <MFRC522.h>
#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>
#include <LiquidCrystal_I2C.h>
WiFiClientSecure client;
LiquidCrystal_I2C lcd(0x27, 20, 4);
#define SS_PIN D4
#define RST_PIN D3    // Configurable, see typical pin layout above


//***********Variable to change*******************
const int trigPin = D8;
const int echoPin = D0;
long duration;
int distanceCm;
int Height;
const int x = 200;
void check();
void LcdClearAndPrint(String text);
void HandleDataFromGoogle(String data);

//***********Variable to change*******************


MFRC522 mfrc522(SS_PIN, RST_PIN);  // Create MFRC522 instance
const char* host = "script.google.com";
const int httpsPort = 443;
const char* fingerprint  = "46 B2 C3 44 9C 59 09 8B 01 B6 F8 BD 4C FB 00 74 91 2F EF F6"; // for https


//***********Things to change*******************
const char* ssid = "Gggg";
const char* password = "12345678";
String GOOGLE_SCRIPT_ID = "AKfycbzdbgBfrC8QbECLqJDmHg5cKvcKbMX0AyFbbS7bNHTsG-amJ4IlR_jbOoQ1_DG9x_e4"; // Replace by your GAS service id
const String unitName = "RFIDreader"; // any name without spaces and special characters
//***********Things to change*******************


void setup() {
  Serial.begin(115200);
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);
  lcd.begin(); // Init with pin default ESP8266 or ARDUINO
  // lcd.begin(0, 2); //ESP8266-01 I2C with pin 0-SDA 2-SCL
  // Turn on the blacklight and print a message.
  lcd.backlight();
  LcdClearAndPrint("Loading");

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  Serial.println("Started");
  Serial.print("Connecting");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  // Initialize serial communications with the PC
  while (!Serial);    // Do nothing if no serial port is opened (added for Arduinos based on ATMEGA32U4)
  SPI.begin();      // Init SPI bus
  mfrc522.PCD_Init();   // Init MFRC522
  delay(4);       // Optional delay. Some board do need more time after init to be ready, see Readme
  mfrc522.PCD_DumpVersionToSerial();  // Show details of PCD - MFRC522 Card Reader details
  Serial.println(F("Scan PICC to see UID, SAK, type, and data blocks..."));
  LcdClearAndPrint("Scan Your Card");
}
byte readCard[4];




void loop() {
  if (!mfrc522.PICC_IsNewCardPresent()) {
    return;
  }

  // Select one of the cards
  // Reset the loop if no new card present on the sensor/reader. This saves the entire process when idle.
  if (!mfrc522.PICC_ReadCardSerial()) {
    return;
  }
  Serial.println(F("Scanned PICC's UID:"));
  String uid = "";
  for (uint8_t i = 0; i < 4; i++) {  //
    readCard[i] = mfrc522.uid.uidByte[i];
    Serial.print(readCard[i], HEX);
    uid += String(readCard[i], HEX);
  }
  Serial.println("");


  LcdClearAndPrint("Please wait...");
  String data = sendData("id=" + unitName + "&uid=" + uid, NULL);
  lcd.clear();
  HandleDataFromGoogle(data);
  mfrc522.PICC_HaltA();
}


String sendData(String params, char* domain) {
  //google scripts requires two get requests
 
  bool needRedir = false;
  if (domain == NULL)
  {
    domain = (char*)host;
    needRedir = true;
    params = "/macros/s/" + GOOGLE_SCRIPT_ID + "/exec?" + params;
  }
  Serial.println(*domain);
  String result = "";
  client.setInsecure();
  Serial.print("connecting to ");
  Serial.println(host);

  if (!client.connect(host, httpsPort)) {
    Serial.println("connection failed");
    return "";
  }

  if (client.verify(fingerprint, domain)) {
  }

  Serial.print("requesting URL: ");
  Serial.println(params);

  client.print(String("GET ") + params + " HTTP/1.1\r\n" +
               "Host: " + domain + "\r\n" +
               "Connection: close\r\n\r\n");

  Serial.println("request sent");
  while (client.connected()) {

    String line = client.readStringUntil('\n');
    //Serial.println(line);
    if (needRedir) {

      int ind = line.indexOf("/macros/echo?user");
      if (ind > 0)
      {
        Serial.println(line);
        line = line.substring(ind);
        ind = line.lastIndexOf("\r");
        line = line.substring(0, ind);
        Serial.println(line);
        result = line;
      }
    }

    if (line == "\r") {
      Serial.println("headers received");
      break;
    }
  }
  while (client.available()) {
    String line = client.readStringUntil('\n');
    if (!needRedir)
      if (line.length() > 6)
        result = line;
    //Serial.println(line);

  }
  if (needRedir)
    return sendData(result, "script.googleusercontent.com");
  else return result;


}
void LcdClearAndPrint(String text)
{
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(text);
}
void HandleDataFromGoogle(String data)
{
  int ind = data.indexOf(":");
  int nextInd = data.indexOf(":", ind + 1);
  String access = data.substring(0, ind);
  String name = data.substring(ind + 1, nextInd);
  String text = data.substring(nextInd + 1, data.length());

  Serial.println(name);
  LcdClearAndPrint(name);
  lcd.setCursor(8, 0);
  lcd.print(text);
  lcd.setCursor(4, 1);
  lcd.print("Your Height");
  lcd.setCursor(5, 2);
  lcd.print("=");
  lcd.setCursor(10, 2);
  lcd.print(" CM");

  if (access == "-1")
  {
    lcd.print(" " + String("denied"));

  }
  else if (access == "any")
  {
    for (int i = 0; i <= 3; i++)
    {
      check();
      delay(1000);

    }
    lcd.setCursor(7, 2);
    lcd.print(Height);
    delay(7000);

    LcdClearAndPrint("Scan your Card");

    //  lcd.setCursor(5, 0); // Sets the location at which subsequent text written to the LCD will be displayed
    //  lcd.print("Your Height");
    //  lcd.setCursor(7, 1);
    //  lcd.print(Result);

    Serial.println(data);
  }
}
void check()
{
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);

  duration = pulseIn(echoPin, HIGH);
  distanceCm = duration * 0.034 / 2;
  Height = (x - distanceCm);
  Serial.println(Height);
}
