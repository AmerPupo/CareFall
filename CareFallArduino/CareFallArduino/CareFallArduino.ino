#include <Wire.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include <ESP8266WiFi.h>
#include <Firebase_ESP_Client.h>


//Provide the token generation process info.
#include "addons/TokenHelper.h"
//Provide the RTDB payload printing info and other helper functions.
#include "addons/RTDBHelper.h"

#define WIFI_SSID "Your_WIFI_SSID"
#define WIFI_PASSWORD "Your_WIFI_Password"

#define DATABASE_URL "Your_Firebase_RealtimeDatabase_URL"
#define API_KEY "Your_Firebase_API_KEY"

// Define Firebase Data object
FirebaseData fbdo;

FirebaseAuth auth;
FirebaseConfig config;
unsigned long timePassed = 0;
unsigned long sendDataPrevMillis = 0;
bool signupOK = false;

Adafruit_MPU6050 mpu;
volatile bool fallDetected = false;
int notifTime;
int fallThreshold = 150; //Change the threshold for your purposes
void setup() {
  Serial.begin(115200);
  while (!Serial)
    delay(10);

  Serial.println("Adafruit MPU6050 Fall Detection");

  if (!mpu.begin()) {
    Serial.println("Failed to find MPU6050 chip");
    while (1) {
      delay(10);
    }
  }

  mpu.setHighPassFilter(MPU6050_HIGHPASS_0_63_HZ);
  mpu.setMotionDetectionThreshold(1);
  mpu.setMotionDetectionDuration(200);
  mpu.setInterruptPinLatch(true);
  mpu.setInterruptPinPolarity(true);
  mpu.setMotionInterrupt(true);
  // connect to wifi.
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("connecting");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(500);
  }
  Serial.println();
  Serial.print("connected: ");
  Serial.println(WiFi.localIP());

  Serial.printf("Firebase Client v%s\n\n", FIREBASE_CLIENT_VERSION);
  /* Assign the api key (required) */
  config.database_url = DATABASE_URL;
  config.api_key = API_KEY;
/* Sign up */
  if (Firebase.signUp(&config, &auth, "", "")){
    Serial.println("ok");
    signupOK = true;
  }
  else{
    Serial.printf("%s\n", config.signer.signupError.message.c_str());
  }

  /* Assign the callback function for the long running token generation task */
  config.token_status_callback = tokenStatusCallback; //see addons/TokenHelper.h
  
  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);
  pinMode(D5, OUTPUT);    // LED
  pinMode(D6, OUTPUT);    // Buzzer
  pinMode(D7, INPUT_PULLUP); // Button
  notifTime=0;
}

void loop() {
  int newNotifTime;
  float calculated = 30;
  if (Firebase.ready() && signupOK && (millis() - sendDataPrevMillis > 1000 || sendDataPrevMillis == 0)){
    sendDataPrevMillis = millis();

  if(Firebase.RTDB.getInt(&fbdo, "/Devices/pQ4a6z8x/DeviceTime")){
    if(fbdo.dataType() == "int"){
      newNotifTime = fbdo.intData();
      if(newNotifTime != notifTime){
        Serial.println("NotifTime:");
        Serial.println(newNotifTime);
        notifTime=newNotifTime;
      }
    }
  }
   else {
      Serial.println(fbdo.errorReason());
    }

  if (fallDetected == false && mpu.getMotionInterruptStatus()) {
    // Get new sensor events with the readings
    sensors_event_t a, g, t;
    mpu.getEvent(&a, &g, &t);
    float Ax = a.acceleration.x;
    float Ay = a.acceleration.y;
    float Az = a.acceleration.z;
    float Gx = a.gyro.x;
    float Gy = a.gyro.y;
    float Gz = a.gyro.z;
    //Use acceleration and orientation of 3 axis to determine if fall was detected
    calculated = sqrt(((abs(Ax) * Gx) * (abs(Ax) * Gx)) + ((abs(Ay) * Gy)*(abs(Ay) * Gy)) + ((abs(Az) * Gz)*(abs(Az) * Gz)));
    Serial.println("calculated = ");
    Serial.println(calculated);
    delay(100);

  }
  if (calculated > fallThreshold) {
    Serial.println("Fall detected!");
    activateAlarm();
  }
}
}
void checkButton() {
  static bool buttonWasPressed = false;
  Serial.println("Button state: " + String(digitalRead(D7)));
  // Check if the button is pressed (HIGH), and it wasn't pressed before
  if (digitalRead(D7) == HIGH && !buttonWasPressed && fallDetected) {
    turnOffAlarm();
    buttonWasPressed = true;
  } else if (digitalRead(D7) == LOW) {
    buttonWasPressed = false;
  }
}

void activateAlarm() {
  fallDetected = true;
  timePassed = millis();
  while(fallDetected){
  digitalWrite(D5, HIGH);  // Turn on LED
  digitalWrite(D6, HIGH); // Turn on Buzzer
  // Serial.println("Time: ");
  // Serial.println((millis()-timePassed)/1000); //Use to check how much time has passed since the beginning of the function
  if(millis()-timePassed>notifTime*1000){
    if(Firebase.RTDB.setBool(&fbdo, "/Devices/pQ4a6z8x/FallDetected", fallDetected)){
      Serial.println("PASSED");
      Serial.println("PATH: " + fbdo.dataPath());
      Serial.println("TYPE: " + fbdo.dataType() + " " + fallDetected);
    }
    else {
      Serial.println("FAILED");
      Serial.println("REASON: " + fbdo.errorReason());
    }
  }
  checkButton();
}
}


void turnOffAlarm() {
  // Turn off LED and Buzzer when the button is pressed and set FallDetected to false on firebase
  fallDetected = false;
   if(Firebase.RTDB.setBool(&fbdo, "/Devices/pQ4a6z8x/FallDetected", fallDetected)){
      Serial.println("PASSED");
      Serial.println("PATH: " + fbdo.dataPath());
      Serial.println("TYPE: " + fbdo.dataType() + " " + fallDetected);
    }
    else {
      Serial.println("FAILED");
      Serial.println("REASON: " + fbdo.errorReason());
    }
  timePassed = 0;
  digitalWrite(D5, LOW);
  digitalWrite(D6, LOW);
}
