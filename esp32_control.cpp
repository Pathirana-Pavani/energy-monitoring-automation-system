#include <WiFi.h>
#include <Firebase_ESP_Client.h>
#include "addons/TokenHelper.h"
#include "addons/RTDBHelper.h"

#define RXD2 16
#define TXD2 17

#define WIFI_SSID "PAVANI"
#define WIFI_PASSWORD "12345678"
#define API_KEY "AIzaSyAQZtJO0eZwK-AGLrzDo234q1Vq2V9626Q"
#define DATABASE_URL "https://smart-power-monitor-281-default-rtdb.asia-southeast1.firebasedatabase.app/"

FirebaseData fbdo;
FirebaseData stream;
FirebaseAuth auth;
FirebaseConfig config;

String lastCommand = "";
String streamPath = "monitoring/command";

unsigned long sendDataPrevMillis = 0;
bool signupOK = false;

void processCommand(String cmd)
{
  if (cmd.startsWith("TOGGLE:") && cmd.length() >= 11)
  {

    int componentID = cmd.charAt(7) - '0';
    String state = cmd.substring(9);
    bool newState = (state == "ON");

    String fullCmd = "TOGGLE:" + String(componentID) + ":" + (newState ? "ON" : "OFF");
    Serial2.println(fullCmd);
    Serial.println("Sent to UNO: " + fullCmd);
  }
  else if (cmd == "TURNOFFALL")
  {
    for (int i = 1; i <= 3; i++)
    {
      String fullCmd = "TOGGLE:" + String(i) + ":OFF";
      Serial2.println(fullCmd);
      Serial.println("Sent to UNO: " + fullCmd);
    }
  }
}

void clearCommandNode()
{
  if (Firebase.RTDB.getString(&fbdo, streamPath))
  {
    if (Firebase.RTDB.deleteNode(&fbdo, streamPath))
    {
      Serial.println("Command cleared (node deleted) from Firebase.");
    }
    else
    {
      Serial.println("Failed to clear command: " + fbdo.errorReason());
    }
  }
  else
  {
    Serial.println("Command already cleared or missing.");
  }
}

void streamCallback(FirebaseStream data)
{
  String cmd = data.stringData();
  Serial.println("Stream callback - new data:");
  Serial.println(cmd);

  if (cmd.length() > 0 && cmd.startsWith("TOGGLE") || cmd == "TURNOFFALL")
  {
    if (cmd != lastCommand)
    {
      lastCommand = cmd;
      processCommand(cmd);
      clearCommandNode();
    }
  }
}

void streamTimeoutCallback(bool timeout)
{
  if (timeout)
  {
    Serial.println("Stream timeout, resuming...");
  }
}

void readDataFromUNO()
{
  while (Serial2.available())
  {
    String data = Serial2.readStringUntil('\n');
    data.trim();

    if (!data.startsWith("DATA:"))
    {
      Serial.println("Ignored non-DATA serial message: " + data);
      return;
    }

    if (data.startsWith("DATA:"))
    {
      data = data.substring(5);
      std::vector<String> parts;
      int lastIdx = 0;

      while (true)
      {
        int comma = data.indexOf(',', lastIdx);
        if (comma == -1)
        {
          parts.push_back(data.substring(lastIdx));
          break;
        }
        parts.push_back(data.substring(lastIdx, comma));
        lastIdx = comma + 1;
      }

      if (parts.size() >= 7)
      {
        float power = parts[0].toFloat();
        float daily = parts[1].toFloat();
        float total = parts[2].toFloat();
        int threshold = parts[6].toInt();

        FirebaseJson json;
        json.set("power", power);
        json.set("dailyEnergy", daily);
        json.set("totalEnergy", total);
        json.set("thresholdLevel", threshold);
        json.set("component1", parts[3] == "ON");
        json.set("component2", parts[4] == "ON");
        json.set("component3", parts[5] == "ON");

        if (!Firebase.RTDB.updateNode(&fbdo, "monitoring", &json))
        {
          Serial.println("Firebase update failed: " + fbdo.errorReason());
        }
        else
        {
          Serial.println("Firebase updated: " + data);
        }
      }
      else
      {
        Serial.println("Invalid DATA received: " + data);
      }
    }
  }
}

void setup()
{
  Serial.begin(115200);
  Serial2.begin(9600, SERIAL_8N1, RXD2, TXD2);
  delay(1000);
  Serial.println("Booting...");

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED)
  {
    Serial.print(".");
    delay(300);
  }
  Serial.println("\nWiFi connected");

  config.api_key = API_KEY;
  config.database_url = DATABASE_URL;
  config.token_status_callback = tokenStatusCallback;
  config.timeout.serverResponse = 10000;
  config.max_token_generation_retry = 5;

  // SSL buffer optimization
  fbdo.setBSSLBufferSize(2048, 512);
  stream.setBSSLBufferSize(2048, 512);
  fbdo.setResponseSize(2048);
  stream.setResponseSize(2048);

  Serial.println("Signing up to Firebase...");
  int retry = 0;
  while (!Firebase.signUp(&config, &auth, "", "") && retry < 5)
  {
    Serial.println("Retrying Firebase sign-up...");
    delay(2000);
    retry++;
  }

  if (Firebase.ready() && auth.token.uid.length() > 0)
  {
    signupOK = true;
    Serial.println("Firebase sign-up successful");
  }
  else
  {
    Serial.printf("Sign-up failed: %s\n", config.signer.signupError.message.c_str());
  }

  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);

  Firebase.RTDB.setReadTimeout(&fbdo, 1000);
  Firebase.RTDB.setwriteSizeLimit(&fbdo, "tiny");

  if (!Firebase.RTDB.beginStream(&stream, streamPath))
  {
    Serial.println("Could not begin stream: " + stream.errorReason());
  }
  Firebase.RTDB.setStreamCallback(&stream, streamCallback, streamTimeoutCallback);
}

void loop()
{
  if (Firebase.ready() && signupOK)
  {
    if (millis() - sendDataPrevMillis > 1000)
    { // 1 second interval
      sendDataPrevMillis = millis();
      readDataFromUNO();
    }
  }
  delay(20);
}
