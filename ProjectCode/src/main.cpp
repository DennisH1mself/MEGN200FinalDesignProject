#include <Arduino.h>
#include <Servo.h>
#include "WiFiS3.h"
#include "ArduinoJson.h"
#include "html.h"
#include <string>
#include <TinyGPS++.h>
#include <SoftwareSerial.h>
// GPS Declarations
static const int TXPin = 11, RXPin = 10;
static const uint32_t GPSBaud = 9600;

TinyGPSPlus gps;
SoftwareSerial ss(RXPin, TXPin); // RX, TX
void displayInfo()
{
  Serial.print(F("Location: "));
  if (gps.location.isValid())
  {
    Serial.print(gps.location.lat(), 6);
    Serial.print(F(","));
    Serial.print(gps.location.lng(), 6);
  }
  else
  {
    Serial.print(F("INVALID"));
  }

  Serial.print(F("  Date/Time: "));
  if (gps.date.isValid())
  {
    Serial.print(gps.date.month());
    Serial.print(F("/"));
    Serial.print(gps.date.day());
    Serial.print(F("/"));
    Serial.print(gps.date.year());
  }
  else
  {
    Serial.print(F("INVALID"));
  }

  Serial.print(F(" "));
  if (gps.time.isValid())
  {
    if (gps.time.hour() < 10)
      Serial.print(F("0"));
    Serial.print(gps.time.hour());
    Serial.print(F(":"));
    if (gps.time.minute() < 10)
      Serial.print(F("0"));
    Serial.print(gps.time.minute());
    Serial.print(F(":"));
    if (gps.time.second() < 10)
      Serial.print(F("0"));
    Serial.print(gps.time.second());
    Serial.print(F("."));
    if (gps.time.centisecond() < 10)
      Serial.print(F("0"));
    Serial.print(gps.time.centisecond());
  }
  else
  {
    Serial.print(F("INVALID"));
  }

  Serial.println();
}
// END GPS Declarations

// MOTOR DECLARATIONS
#define MOTOR_PIN 9
#define SERVO_PIN 10
int motor_pin = MOTOR_PIN;
Servo myServo;
int servoAngle = 0; // Initial angle for the servo
// END MOTOR DECLARATIONS

// WiFi Credentials
#define SECRET_SSID "DennisNet"
#define SECRET_PASS "dennis_is_a_menace"
#define US_TRIG_PIN 2
#define US_ECHO_PIN 3
char ssid[] = SECRET_SSID; // your network SSID (name)
char pass[] = SECRET_PASS; // your network password (use for WPA, or use as key for WEP)
int keyIndex = 0;          // your network key index number (needed only for WEP)
int status = WL_IDLE_STATUS;
// END WiFi Credentials

int distance = 0; // Variable to store the distance value
int duration = 0; // Variable to store the duration value

WiFiServer server(80);
class HttpRequest
{
public:
  String method;
  String path;
  String query;
  JsonDocument body;
  String contentType;
  String userAgent;
  String host;

  HttpRequest(const String &rawData)
  {
    parse(rawData);
  }

  void parse(const String &rawData)
  {
    int methodEnd = rawData.indexOf(' ');
    method = rawData.substring(0, methodEnd);

    int pathStart = methodEnd + 1;
    int pathEnd = rawData.indexOf(' ', pathStart);
    String fullPath = rawData.substring(pathStart, pathEnd);

    int queryStart = fullPath.indexOf('?');
    if (queryStart != -1)
    {
      path = fullPath.substring(0, queryStart);
      query = fullPath.substring(queryStart + 1);
    }
    else
    {
      path = fullPath;
      query = "";
    }

    int bodyStart = rawData.indexOf("\r\n\r\n");
    if (bodyStart != -1)
    {
      String bodyString = rawData.substring(bodyStart + 4);
      if (!bodyString.isEmpty())
      {
        DeserializationError error = deserializeJson(body, bodyString);
        if (error)
        {
          Serial.print(F("deserializeJson() failed: "));
          Serial.println(error.f_str());
          body.clear(); // Clear the body to avoid using invalid data
        }
      }
      else
      {
        Serial.println(F("deserializeJson() failed: EmptyInput"));
        body.clear();
      }
    }
    else
    {
      Serial.println(F("No body found in the request"));
      body.clear();
    }
    /*
    if (bodyStart != -1) {
      body = rawData.substring(bodyStart + 4);
    } else {
      body = "";
    }*/

    contentType = extractHeaderValue(rawData, "Content-Type: ");
    userAgent = extractHeaderValue(rawData, "User-Agent: ");
    host = extractHeaderValue(rawData, "Host: ");
  }

private:
  String extractHeaderValue(const String &rawData, const String &headerName)
  {
    int headerStart = rawData.indexOf(headerName);
    if (headerStart != -1)
    {
      int valueStart = headerStart + headerName.length();
      int valueEnd = rawData.indexOf("\r\n", valueStart);
      return rawData.substring(valueStart, valueEnd);
    }
    return "";
  }
};

class HttpResponse
{
public:
  int statusCode;
  String statusMessage;
  String contentType;
  String body;

  HttpResponse(int code = 200, const String &message = "OK", const String &type = "application/json", const String &content = "")
      : statusCode(code), statusMessage(message), contentType(type), body(content) {}

  String toString() const
  {
    String response = "HTTP/1.1 " + String(statusCode) + " " + statusMessage + "\r\n";
    response += "Content-Type: " + contentType + "\r\n";
    response += "Content-Length: " + String(body.length()) + "\r\n";
    response += "Connection: close\r\n\r\n";
    response += body;
    return response;
  }
};

void printWiFiStatus()
{
  // print the SSID of the network you're attached to:
  Serial.print("SSID: ");
  Serial.println(WiFi.SSID());

  // print your WiFi shield's IP address:
  IPAddress ip = WiFi.localIP();
  Serial.print("IP Address: ");
  Serial.println(ip);

  // print where to go in a browser:
  Serial.print("To see this page in action, open a browser to http://");
  Serial.println(ip);
}
void updateDistance()
{
  // Update the distance value using ultrasonic sensor
  digitalWrite(US_TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(US_TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(US_TRIG_PIN, LOW);
  duration = pulseIn(US_ECHO_PIN, HIGH);

  // Ensure duration is valid before calculating distance
  if (duration > 0)
  {
    distance = duration * 0.034 / 2; // Calculate distance in cm
  }
  else
  {
    distance = -1; // Indicate an invalid reading
  }
  Serial.print("Distance: ");
  if (distance >= 0)
  {
    Serial.println(distance);
  }
  else
  {
    Serial.println("Invalid reading");
  }
}
void respond(HttpRequest &request, WiFiClient &client)
{
  if (request.method == "GET" && request.path == "/panel/")
  {
    // Handle GET request for the root path
    String panelContent = String(htmlContent.c_str()); // Use the HTML content defined in html.h
    client.println(HttpResponse(200, "OK", "text/html", panelContent).toString());
  }

  else if (request.method == "POST" && request.path == "/control/motor")
  {
    // Handle POST request for motor control
    if (request.body.containsKey("motor"))
    {
      int motorValue = request.body["motor"].as<int>();
      Serial.print("Motor value received: ");
      Serial.println(motorValue);
      // Add your motor control logic here
      client.println(HttpResponse(200, "OK", "application/json", "{\"status\": \"Motor value received\"}").toString());
      analogWrite(motor_pin, motorValue); // Set the motor speed (0-255)
    }
    else
    {
      client.println(HttpResponse(400, "Bad Request", "application/json", "{\"error\": \"Invalid motor value\"}").toString());
    }
  }
  else if (request.method == "POST" && request.path == "/control/servo")
  {
    // Handle POST request for servo control
    if (request.body.containsKey("servo"))
    {
      int servoValue = request.body["servo"].as<int>();
      Serial.print("Servo value received: ");
      Serial.println(servoValue);
      // Add your servo control logic here
      client.println(HttpResponse(200, "OK", "application/json", "{\"status\": \"Servo value received\"}").toString());
      myServo.write(servoValue); // Set the servo angle (0-180)
    }
    else
    {
      client.println(HttpResponse(400, "Bad Request", "application/json", "{\"error\": \"Invalid servo value\"}").toString());
    }
  }
  /*else if (request.method == "GET" && request.path == "/control/distance")
  {
    updateDistance(); // Update the distance value using ultrasonic sensor
    // Handle POST request for servo control
    Serial.print("Distance value requested: ");
    Serial.println(distance);
    JsonDocument response;
    response["distance"] = distance;
    String responseBody;
    serializeJson(response, responseBody);
    client.println(HttpResponse(200, "OK", "application/json", responseBody).toString());
  }*/
  else
  {
    // Handle other requests (e.g., POST, PUT, DELETE)
    client.println(HttpResponse(404, "Not Found", "application/json", "{\"error\": \"Not Found\"}").toString());
  }
  /*
  // Create a JSON response
  JsonDocument responseBody;
  responseBody["message"] = "Hello, World!";
  String responseBodyString;
  serializeJson(responseBody, responseBodyString);

  // Send the HTTP response
  client.println(HttpResponse(200, "OK", "application/json", responseBodyString).toString());*/
}
// END HTTP Declarations

void setup()
{
  Serial.begin(9600);
  while (!Serial)
  {
    delay(1);
  }
  // Initialize Motors
  pinMode(motor_pin, OUTPUT);
  pinMode(US_TRIG_PIN, OUTPUT);
  pinMode(US_ECHO_PIN, INPUT);
  myServo.attach(SERVO_PIN);
  myServo.write(servoAngle);

  // Initialize GPS
  /*ss.begin(GPSBaud);
  if (WiFi.status() == WL_NO_MODULE)
  {
    Serial.println("Communication with WiFi module failed!");
    while (true)
      ;
  }*/

  String fv = WiFi.firmwareVersion();
  if (fv < WIFI_FIRMWARE_LATEST_VERSION)
  {
    Serial.println("Please upgrade the firmware");
  }

  WiFi.config(IPAddress(192, 48, 56, 2));

  Serial.print("Creating access point named: ");
  Serial.println(ssid);

  status = WiFi.beginAP(ssid, pass);
  if (status != WL_AP_LISTENING)
  {
    Serial.println("Creating access point failed");
    while (true)
      ;
  }

  // delay(10000);
  delay(1000); // Give the WiFi time to connect
  server.begin();
  printWiFiStatus();
}
void loop()
{

  /*if (ss.available()) {
    while (ss.available()) {
      char c = ss.read();
      Serial.print(c); // Debug: Print raw GPS data
      gps.encode(c);
    }
    if (gps.location.isUpdated()) {
      displayInfo();
    }
  } else {
    Serial.println(F("No data available from GPS module."));
  }*/

  // Update the distance value using ultrasonic sensor
  // compare the previous status to the current status
  if (status != WiFi.status())
  {
    // it has changed update the variable
    status = WiFi.status();

    if (status == WL_AP_CONNECTED)
    {
      // a device has connected to the AP
      Serial.println("Device connected to AP");
    }
    else
    {
      // a device has disconnected from the AP, and we are back in listening mode
      Serial.println("Device disconnected from AP");
    }
  }

  WiFiClient client = server.available(); // listen for incoming clients

  if (client)
  {                                       // if you get a client,
    Serial.println(" - CLIENT ADDED - "); // print a message out the serial port
    String data = "";                     // make a String to hold incoming data from the client
    while (client.connected())
    {
      if (client.available())
      {
        char c = client.read();
        data += c;

        // Detect the end of the HTTP headers (double CRLF)
        if (data.indexOf("\r\n\r\n") != -1)
        {
          // Check if there's a Content-Length header to read the body
          String contentLengthHeader = "Content-Length: ";
          int contentLengthIndex = data.indexOf(contentLengthHeader);
          if (contentLengthIndex != -1)
          {
            int valueStart = contentLengthIndex + contentLengthHeader.length();
            int valueEnd = data.indexOf("\r\n", valueStart);
            int contentLength = data.substring(valueStart, valueEnd).toInt();

            // Read the body based on Content-Length
            while (data.length() < data.indexOf("\r\n\r\n") + 4 + contentLength)
            {
              if (client.available())
              {
                data += (char)client.read();
              }
            }
          }
          break;
        }
      }
    }
    // Serial.println(data);

    // Parse the HTTP request
    /*HttpRequest request(data);
    Serial.println("Method: " + request.method);
    Serial.println("Path: " + request.path);
    Serial.println("Query: " + request.query);
    Serial.println("Body: " + request.body.as<String>());
    Serial.println("Content-Type: " + request.contentType);
    Serial.println("User-Agent: " + request.userAgent);
    Serial.println("Host: " + request.host);
    // close the connection:
    JsonDocument responseBody;
    responseBody["message"] = "Hello, World!";
    String responseBodyString;
    serializeJson(responseBody, responseBodyString);

    client.println(HttpResponse(200, "OK", "application/json", responseBodyString).toString());*/
    // Handle the request and send a response
    HttpRequest request(data);
    respond(request, client);

    client.stop();
    Serial.println(" - CLIENT REMOVED - ");
  }
}
