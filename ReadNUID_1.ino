#include <SPI.h>
#include <MFRC522.h>
#include <WiFiManager.h> 
#include <WiFi.h>
#include <Wire.h> 
#include <LiquidCrystal_I2C.h>
#include <NTPClient.h>
#include <HTTPClient.h>


const char* server = "http://192.168.1.101:8080/data"; // BACKEND

//---------------RC522 PIN------------------------------------------
#define SS_PIN 5
#define RST_PIN 4
MFRC522 rfid(SS_PIN, RST_PIN); // Instance of the class
MFRC522::MIFARE_Key key; 
//------------------------------------------------------------------------

#define buzzer 26 // buzzer pinout

//---------LED's------------------------------------------------------------
#define LED_R 32 //LED_RED
#define LED_B 33 //LED_BLUE
#define LED_G 25 //LED_GREEN
//------------------------------------------------------------------------------


//-----------------------CLOCK-------------------------------------------------
String formattedDate;
String dayStamp;
String timeStamp;
WiFiUDP ntpUDP;
NTPClient ntp(ntpUDP);
//------------------------------------------------------------------------------


#define TRIGGER_PIN 15 // select which pin will trigger the configuration portal when set to LOW-----------
int timeout = 90; // seconds to run for
//-------------------------------------------------------------------------------------- 

LiquidCrystal_I2C lcd(0x27, 16, 2); // Adress LCD


// Init array that will store new NUID 
char strUID[32] = "";
//--------------------------------

//---------- variable wifimanager---
WiFiManager wm;
//---------------------------------------
void setup() { 
  Serial.begin(115200);

//-------LCD-------------------------------------------------------
  lcd.begin(); // initialize the LCD
  lcd.backlight();// Turn on the blacklight and print a message.

//-----RFID--------------------------------------------------------
  SPI.begin(); // Init SPI bus
  rfid.PCD_Init(); // Init MFRC522 
  pinMode(buzzer,OUTPUT);
//---------------------------------------------------------------------------------
  
//--------------WIFI SETUP-----------------------------------------------------------
     pinMode(LED_G,OUTPUT);  //LED_GREEN
     pinMode(LED_R,OUTPUT); //LED_RED
     pinMode(LED_B,OUTPUT); //LED_BLUE
     pinMode(TRIGGER_PIN, INPUT_PULLUP); 
    WiFi.mode(WIFI_STA); // explicitly set mode, esp defaults to STA+AP    
    wm.setConfigPortalBlocking(false);
    wm.setConfigPortalTimeout(1);
    //automatically connect using saved credentials if they exist
    //If connection fails it starts an access point with the specified name
    wm.autoConnect("AutoConnectAP");
//----------------------------------------------------------------------------------

//-----------------------CLOCK-------------------------------------------------------
ntp.begin();
ntp.setTimeOffset(-10800); //GMT em segundos // -3 = -10800 (BRASIL)
//------------------------------------------------------------------------------------

}
 
void loop() {

    wm.process();
    WifiConnect();
    

//  new card present on the sensor/reader
  if ( rfid.PICC_IsNewCardPresent()){
    // Verify if the NUID has been readed
    if (  rfid.PICC_ReadCardSerial()){
        getUID(); //Function GetUID
      }  
    } 
}

//-----------------------WIFI PORTAL------------------------------------------------------------------------------------
void WifiConnect(){
    if (WiFi.status() == WL_CONNECTED){   // if connected Green led on and clock on LCD
    digitalWrite(LED_R,LOW);
    digitalWrite(LED_G,HIGH);
    digitalWrite(LED_B,LOW);
    clock_PM();
    }
   else {                                //else RED LED On
    digitalWrite(LED_R,HIGH); 
    digitalWrite(LED_G,LOW);
    digitalWrite(LED_B,LOW);

      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("not connected");   
    }
  
     if ( digitalRead(TRIGGER_PIN) == LOW) { //IF PIN  LOW  Configuration mode of wifi
    WiFiManager wm;    
     digitalWrite(LED_R,LOW);
    digitalWrite(LED_G,LOW);
    digitalWrite(LED_B,HIGH);
   
    wm.resetSettings();//reset settings 
    wm.setConfigPortalTimeout(timeout);// set configportal timeout 90sec
     
     
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Connect Wifi:");
      lcd.setCursor(0, 1);
      lcd.print("PointManager");
     

    if (!wm.startConfigPortal("PointManager-P4tech")) {
      Serial.println("failed to connect and hit timeout");

      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Fail to connect");
      lcd.setCursor(0, 1);
      lcd.print("and hit timeout");
      delay(3000);
      
      //reset and try again, or maybe put it to deep sleep
      ESP.restart();
      delay(5000);
    }

    //if you get here you have connected to the WiFi
    Serial.println("connected...yeey :)");

      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Connected :) ");
      digitalWrite(LED_R,LOW);
      digitalWrite(LED_G,HIGH);
      digitalWrite(LED_B,LOW);
      delay(4000);

  }  
  }
//---------------------------------------------------------------------------------------------------------------------------------------------




//--------------------------------------------------------------GET UID --------------------------------------------------------------------------------
void getUID(){  
 
    // Store NUID into nuidPICC array
    for (byte i = 0; i < 4; i++) {
      byte nib1 = (rfid.uid.uidByte[i] >> 4) & 0x0F;
      byte nib2 = (rfid.uid.uidByte[i] >> 0) & 0x0F;
      strUID[i*2+0] = nib1  < 0xA ? '0' + nib1  : 'A' + nib1  - 0xA;
      strUID[i*2+1] = nib2  < 0xA ? '0' + nib2  : 'A' + nib2  - 0xA;
    }
    strUID[4*2] = '\0';

    
    send_id(strUID);
    Serial.println(F("The NUID tag is:"));
    Serial.println(strUID);


  // Halt PICC
  rfid.PICC_HaltA();

  // Stop encryption on PCD
  rfid.PCD_StopCrypto1();  

  digitalWrite(buzzer,HIGH);
  delay(200);
  digitalWrite(buzzer,LOW);
  delay(100);
  digitalWrite(buzzer,HIGH);
  delay(200);
  digitalWrite(buzzer,LOW);
   delay(5000);
}

//---------------------------------------------------------------------------------------------------------------------------------------------------------------

//---------------------------------------CLOCK-------------------------------------------------------------------------------------------
void clock_PM(){
  
  if (ntp.update()) {
     
  
  // The formattedDate comes with the following format:
  // 2018-05-28T16:00:13Z
  // We need to extract date and time
  formattedDate = ntp.getFormattedDate();

  // Extract date
  int splitT = formattedDate.indexOf("T");
  dayStamp = formattedDate.substring(0, splitT);
  Serial.print("DATE: ");
  Serial.println(dayStamp);
  // Extract time
  timeStamp = formattedDate.substring(splitT+1, formattedDate.length()-1);
  Serial.print("HOUR: ");
  Serial.println(timeStamp);
   lcd.clear();
   lcd.setCursor(0, 0);
   lcd.print(dayStamp);
   lcd.setCursor(0, 1);
   lcd.print(timeStamp);   
  delay(1000);

  } else {
    Serial.println("!Erro ao atualizar NTP!\n");
  }
  }

 //-----------------------------------SEND_ID--------------------------------------------

 void send_id( char*strUID ){

      WiFiClient client;
       HTTPClient http;
            
        http.begin(client, server);
        http.addHeader("Content-Type", "text/plain");


        int httpResponseCode = http.GET();
         if(httpResponseCode>0){
            
            Serial.println(strUID);  

            int responseCode = http.POST(strUID);
            String responde = http.getString();
            
           if(responde == "IDinv")
           {

               lcd.clear();
               lcd.setCursor(0, 0);
               lcd.print("ID INVALIDO");
               lcd.setCursor(0, 1);
               lcd.print(strUID);
            
            }
           else{
               lcd.clear();
               lcd.setCursor(0, 0);
               lcd.print(responde);
            }
            http.end();
             }
            else {          
              Serial.println("erro conexão com servidor");
               lcd.clear();
               lcd.setCursor(0, 0);
               lcd.print("ERROR SERVER");  
              }
   
 }
 
