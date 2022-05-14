
// create the display object
#define FPSTR(pstr_pointer) (reinterpret_cast<const __FlashStringHelper *>(pstr_pointer))
#define F(string_literal) (FPSTR(PSTR(string_literal)))
#define SCREEN_WIDTH 128   // OLED display width, in pixels
#define SCREEN_X_OFFSET 10 // OLED display width, in pixels
#define GRAPH_WIDTH (SCREEN_WIDTH - SCREEN_X_OFFSET)
#define X_INC 10
#define SCREEN_HEIGHT 64  // OLED display height, in pixels
#define SCREEN_Y_OFFSET 2 // OLED display width, in pixels
#define GRAPH_HEIGHT (SCREEN_HEIGHT - SCREEN_Y_OFFSET)

#define ADC_LOOP_PERIOD 500

#define VOLTAGE_MAX 15.00  // Voltage
#define VOLTAGE_MIN 13.40  // Voltage
#define VOLTAGE_CUTOFF 9.6 // Voltage 9.6 for fast discharging
#define VOLTAGE_MAX_DURATION 300000
#define VOLTAGE_MIN_DURATION 1000
#define ALERT_VOLTAGE 8.8 // cm

#define DISTANCE_MAX 500 // cm
#define DISTANCE_MAX_DURATION DISTANCE_MAX / 0.034 * 2
#define ALERT_DISTANCE 10 // cm

#define i2caddress 0x4b
#define i2crate 1000000
// #define I2CSDA      2// SDA Pin
// #define I2CSCL      0// SCL Pin
/*

  This program provides functions to draw:
  1. cartesian graph
13.72V /4.42 = 3.1
  Pin settings

  Arduino   device
  5V        Vin
  GND       GND

  ....
  A0        read an input voltage from a pot across Vcc and Gnd (just for demo purposes)
  D1        SCL on the display. use dedicated pin on board if provided
  D2        SDA on the display. or use dedicated pin on board if provided
  D3
  D4
  D5        Trigger - distanc
  D6        Echo - distance
  D7        Buzzer
  D8        Buzzer

*/
#include <avr/pgmspace.h>
#include <Ethernet.h>
#include <PubSubClient.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Wire.h>
// Wifi ultrasonic reading using nodemcu
// made by Dhananjay
// cantact adhanu99.blogspot.com

#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>

#include "index.h" //Our HTML webpage contents with javascripts
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>

#define READ_PIN A0
#define OLED_RESET LED_BUILTIN
#define LOGO16_GLCD_HEIGHT 16
#define LOGO16_GLCD_WIDTH 16
#define MSG_BUFFER_SIZE 5

// Replace with your network credentials
const char *ssid = "du_Toit";
const char *password = "Master101!";
const char *mqtt_server = "192.168.0.109";

const int lcdSda = D1;
const int lcdScl = D2;
const int trigPin = D5;
const int echoPin = D6;
const int buzzerPos = D7;
const int buzzerNeg = D8;

// defines variables
long duration;
double distance = DISTANCE_MAX;
double distance_ma_alpha = 0.1;
double distance_loop_delay = 60000;
double distance_ma = DISTANCE_MAX;
double distance_alert = ALERT_DISTANCE;

ESP8266WebServer server(80); // instantiate server at port 80 (http port)

WiFiClient espClient;
PubSubClient client(espClient);
IPAddress remote_mqtt_addr(192, 168, 0, 109);
unsigned long lastMsg = 0;
unsigned long lastMsgDistance = 0;
unsigned long lastMsgVoltage = 0;
char msgVolt[MSG_BUFFER_SIZE];
char msgTemp[MSG_BUFFER_SIZE];
int value = 0;

String htmlPage = "";
String webPage, notice;

// create what ever variables you need
double voltage_loop_delay = VOLTAGE_MAX_DURATION;
double voltage_alert = ALERT_VOLTAGE;
double volts;
volatile double prevVolts = 0.0;
volatile double diffVolts = 0.0;
double diffVolts_ma = 0.0;
double diffVolts_ma_alpha = 0.1;
double bvolts;
double x, y;
double minVoltage = VOLTAGE_MIN;

double remaining_seconds_battery = 0.0;

int temperature;

// these are a required variables for the graphing functions
bool Redraw4 = true;
double ox, oy;

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

void callback(char *topic, byte *payload, unsigned int length)
{
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i = 0; i < length; i++)
  {
    Serial.print((char)payload[i]);
  }
  Serial.println();
}

void reconnect()
{
  // Loop until we're reconnected
  while (!client.connected())
  {
    Serial.print("Attempting MQTT connection...");
    Serial.println("MQTT broker:" + String(mqtt_server));
    // Create a random client ID
    String clientId = "ESP8266Client-";
    clientId += String(random(0xffff), HEX);
    // Attempt to connect
    if (client.connect(clientId.c_str(), "ruan", "ruan"))
    {
      Serial.println("connected");
      // Once connected, publish an announcement...
      // snprintf(msg, MSG_BUFFER_SIZE, "%d", 0);
      // client.publish("mqtt/distance1", "0");
      // ... and resubscribe
      // client.subscribe("inTopic");
    }
    else
    {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

void setup_ota()
{
  ArduinoOTA.onStart([]()
                     {
    String type;
    if (ArduinoOTA.getCommand() == U_FLASH) {
      type = "sketch";
    } else { // U_FS
      type = "filesystem";
    }

    // NOTE: if updating FS this would be the place to unmount FS using FS.end()
    Serial.println("Start updating " + type); });
  ArduinoOTA.onEnd([]()
                   { Serial.println("\nEnd"); });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total)
                        { Serial.printf("Progress: %u%%\r", (progress / (total / 100))); });
  ArduinoOTA.onError([](ota_error_t error)
                     {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) {
      Serial.println("Auth Failed");
    } else if (error == OTA_BEGIN_ERROR) {
      Serial.println("Begin Failed");
    } else if (error == OTA_CONNECT_ERROR) {
      Serial.println("Connect Failed");
    } else if (error == OTA_RECEIVE_ERROR) {
      Serial.println("Receive Failed");
    } else if (error == OTA_END_ERROR) {
      Serial.println("End Failed");
    } });
  ArduinoOTA.begin();
  Serial.println("Ready");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
}

void setup_wifi()
{

  delay(10);
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  // Wait for connection
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(1000);
    Serial.print(".");
  }
  Serial.println("");
  Serial.print("Connected to ");
  Serial.println(ssid);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  server.on("/", handleRoot); // Which routine to handle at root location. This is display page
  server.on("/readADC", handleADC);
  server.on("/readTemperature", handleTemperature);
  server.on("/readRemainingBattSeconds", handleBattRemaining);
  server.on("/readVoltageDiff", handleVoltageDifference);
  server.on("/readSamplingTime", handleSamplingTime);
  server.on("/action_page", handleForm); // form action is handled here

  server.begin();
  Serial.println("Web server started!");
}

//===============================================================
// This routine is executed when you open its IP in browser
//===============================================================
void handleRoot()
{
  // String s = MAIN_page; //Read HTML contents
  server.send(200, "text/html", FPSTR(MAIN_page)); // Send web page
}

void handleBattRemaining()
{
  server.send(200, "text/plane", String(remaining_seconds_battery));
}

void handleADC()
{
  // read some values and convert to volts
  server.send(200, "text/plane", String(volts)); // Send ADC value only to client ajax request
}

void handleTemperature()
{
  server.send(200, "text/plane", String(temperature));
}

void handleVoltageDifference()
{
  server.send(200, "text/plane", String(diffVolts_ma, 4)); // Send ADC value only to client ajax request
}

void handleSamplingTime()
{
  server.send(200, "text/plane", String(voltage_loop_delay)); // Send Sampling time value only to client ajax request
}

void handleForm()
{
  String inputAlertDistance = server.arg("inputAlertDistance");
  String inputAlertVoltage = server.arg("inputAlertVoltage");
  String movingAlpha = server.arg("movingAlpha");
  String distanceLoopDelay = server.arg("distanceLoopDelay");
  String voltageLoopDelay = server.arg("voltageLoopDelay");

  if (inputAlertDistance.length() > 0)
  {
    Serial.print("Alert Distance Input:");
    Serial.println(inputAlertDistance);
    distance_alert = inputAlertDistance.toDouble();

    String s = "<div>inputAlertDistance set to " + inputAlertDistance + " or " + String(distance_alert) + "</div><a href='/'> Go Back </a>";
    server.send(200, "text/html", s); // Send web page
  }

  if (inputAlertVoltage.length() > 0)
  {
    Serial.print("Alert Voltage Input:");
    Serial.println(inputAlertVoltage);
    voltage_alert = inputAlertVoltage.toDouble();

    String s = "<div>inputAlertVoltage set to " + inputAlertVoltage + " or " + String(voltage_alert) + "</div><a href='/'> Go Back </a>";
    server.send(200, "text/html", s); // Send web page
  }

  if (movingAlpha.length() > 0)
  {
    Serial.print("movingAlpha:");
    Serial.println(movingAlpha);
    diffVolts_ma_alpha = movingAlpha.toDouble();

    String s = "<div>movingAlpha set to " + movingAlpha + " or " + String(diffVolts_ma_alpha) + "</div><a href='/'> Go Back </a>";
    server.send(200, "text/html", s); // Send web page
  }

  if (distanceLoopDelay.length() > 0)
  {
    Serial.print("distanceLoopDelay:");
    Serial.println(distanceLoopDelay);
    distance_loop_delay = distanceLoopDelay.toDouble();

    String s = "<div>distanceLoopDelay set to " + distanceLoopDelay + " or " + String(distance_loop_delay) + "</div><a href='/'> Go Back </a>";
    server.send(200, "text/html", s); // Send web page
  }

  if (voltageLoopDelay.length() > 0)
  {
    Serial.print("voltageLoopDelay:");
    Serial.println(voltageLoopDelay);
    voltage_loop_delay = voltageLoopDelay.toDouble();

    String s = "<div>voltageLoopDelay set to " + voltageLoopDelay + " or " + String(voltage_loop_delay) + "</div><a href='/'> Go Back </a>";
    server.send(200, "text/html", s); // Send web page
  }
}

void setupDistanceWifi(void)
{
  pinMode(trigPin, OUTPUT); // Sets the trigPin as an Output
  pinMode(echoPin, INPUT);

  delay(1000);
}

void setupMqtt()
{
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
}

void setupTemperature()
{
  // Wire.begin(I2CSDA, I2CSCL);
  // Wire.setClock(i2crate);
}

void setup()
{

  Serial.begin(115200);
  setup_wifi();
  setup_ota();
  // initialize the display
  // note you may have to change the address
  // the most common are 0X3C and 0X3D

  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  // if you distribute your code, the adafruit license requires you show their splash screen
  // otherwise clear the video buffer memory then display
  display.display();
  // delay(2000);
  display.clearDisplay();
  display.display();

  // setupDistanceWifi();
  // establish whatever pin reads you need
  pinMode(A0, INPUT);

  // setup buzzer
  pinMode(buzzerPos, OUTPUT); // Sets the trigPin as an Output
  pinMode(buzzerNeg, OUTPUT); // Sets the trigPin as an Output
  digitalWrite(buzzerPos, LOW);
  digitalWrite(buzzerNeg, LOW);

  setupTemperature();

  setupMqtt();
}

void loopTemperature()
{
  // temperature in a byte
  Wire.beginTransmission(i2caddress);
  // start the transmission
  Wire.write(0);
  Wire.requestFrom(i2caddress, 1);
  if (Wire.available())
  {
    temperature = Wire.read();
    Serial.println(temperature);
  }
}

void loopMqtt()
{
  if (!client.connected())
  {
    reconnect();
  }
  client.loop();

  unsigned long now = millis();
  if ((now - lastMsgVoltage) > voltage_loop_delay) //(DISTANCE_MAX_DURATION / 1000)
  {
    lastMsgVoltage = now;

    sprintf(msgVolt, "%.2f", volts);
    client.publish("mqtt/voltage1", msgVolt);
    Serial.println("voltage: " + String(msgVolt));
    
    sprintf(msgTemp, "%i", temperature);
    client.publish("mqtt/temperature1", msgTemp);
    Serial.println("temperature: " + String(msgTemp));
  }
}

void loopAdc()
{
  // read some values and convert to volts
  bvolts = analogRead(A0);
  volts = (bvolts / 66);

  if (prevVolts == 0.0)
  { // initial condition
    prevVolts = volts;
    diffVolts_ma = 0.0;
  }

  diffVolts = abs(volts - prevVolts);
  prevVolts = volts;
  diffVolts_ma = diffVolts_ma_alpha * diffVolts + (1 - diffVolts_ma_alpha) * diffVolts_ma;
  if (diffVolts_ma > 0.05) // || diffVolts < -0.1)
  {
    voltage_loop_delay = VOLTAGE_MIN_DURATION;
    minVoltage = VOLTAGE_CUTOFF;
  }
  else
  {
    voltage_loop_delay = VOLTAGE_MAX_DURATION;
    minVoltage = VOLTAGE_MIN;
  }

  remaining_seconds_battery = (volts - VOLTAGE_CUTOFF) / diffVolts_ma * voltage_loop_delay;

  Serial.print("Voltage: ");
  Serial.println(volts);

  if (volts < voltage_alert)
  {
    digitalWrite(buzzerPos, HIGH);
  }
  else
  {
    digitalWrite(buzzerPos, LOW);
  }

  //    DrawCGraph(display, x++, bvolts, 30, 50, 75, 30, 0, 100, 25, 0, 6000, 512, 0, "cm vs Seconds", Redraw4);
  x = x + X_INC;
  DrawCGraph(display, x, volts, SCREEN_X_OFFSET, GRAPH_HEIGHT, GRAPH_WIDTH, GRAPH_HEIGHT, 0, GRAPH_WIDTH, X_INC, minVoltage, VOLTAGE_MAX, 1, 0, "m vs Seconds", Redraw4);
  if (x >= GRAPH_WIDTH)
  {
    x = 0;
    Redraw4 = true;
  }
}

void loopDistance()
{
  // Clears the trigPin
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  // Sets the trigPin on HIGH state for 10 micro seconds
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(15);
  digitalWrite(trigPin, LOW);
  // Reads the echoPin, returns the sound wave travel time in microseconds
  duration = pulseIn(echoPin, HIGH, DISTANCE_MAX_DURATION);
  // Calculating the distance
  distance = duration * 0.034 / 2;
  if (distance == 0.0)
  {
    distance = DISTANCE_MAX;
  }
  distance_ma = distance_ma_alpha * distance + (1 - distance_ma_alpha) * distance_ma;
  // Prints the distance on the Serial Monitor
  Serial.print("Distance: ");
  Serial.print(distance);
  Serial.print(", MA: ");
  Serial.println(distance_ma);

  // if (distance_ma < distance_alert)
  // {
  //   digitalWrite(buzzerPos, HIGH);
  // }
  // else
  // {
  //   digitalWrite(buzzerPos, LOW);
  // }

  // //    DrawCGraph(display, x++, bvolts, 30, 50, 75, 30, 0, 100, 25, 0, 6000, 512, 0, "cm vs Seconds", Redraw4);
  // x = x + X_INC;
  // DrawCGraph(display, x, distance_ma / 1000.0, SCREEN_X_OFFSET, GRAPH_HEIGHT, GRAPH_WIDTH, GRAPH_HEIGHT, 0, GRAPH_WIDTH, X_INC, 0, DISTANCE_MAX / 1000.0, 1, 0, "m vs Seconds", Redraw4);
  // if (x >= GRAPH_WIDTH)
  // {
  //   x = 0;
  //   Redraw4 = true;
  // }
}

unsigned long OldTime;
unsigned long counter;
void loop(void)
{
  unsigned long now = millis();
  if ((now - lastMsg) > ADC_LOOP_PERIOD)
  {
    lastMsg = now;

    // loopDistance();
    loopAdc();
    loopTemperature();
  }
  loopMqtt();
  server.handleClient();
  ArduinoOTA.handle();
}

/*

  function to draw a cartesian coordinate system and plot whatever data you want
  just pass x and y and the graph will be drawn huge arguement list

  &display to pass the display object, mainly used if multiple displays are connected to the MCU
  x = x data point
  y = y datapont
  gx = x graph location (lower left)
  gy = y graph location (lower left)
  w = width of graph
  h = height of graph
  xlo = lower bound of x axis
  xhi = upper bound of x asis
  xinc = division of x axis (distance not count)
  ylo = lower bound of y axis
  yhi = upper bound of y asis
  yinc = division of y axis (distance not count)
  title = title of graph
  &redraw = flag to redraw graph on fist call only

*/

void DrawCGraph(Adafruit_SSD1306 &d, double x, double y, double gx, double gy, double w, double h, double xlo, double xhi, double xinc, double ylo, double yhi, double yinc, double dig, String title, boolean &Redraw)
{

  double i;
  double temp;
  int rot, newrot;

  if (y <= ylo)
  {
    y = ylo;
  }

  if (Redraw == true)
  {
    Redraw = false;
    //    d.fillRect(0, 0,  127 , 16, SSD1306_WHITE);
    //    d.setTextColor(SSD1306_BLACK, SSD1306_WHITE);
    //    d.setTextSize(1);
    //    d.setCursor(2, 4);
    //    d.println(title);
    ox = (x - xlo) * (w) / (xhi - xlo) + gx;
    oy = (y - ylo) * (gy - h - gy) / (yhi - ylo) + gy;
    // draw y scale
    d.setTextSize(1);
    d.setTextColor(SSD1306_WHITE, SSD1306_BLACK);
    for (i = ylo; i <= yhi; i += yinc)
    {
      // compute the transform
      // note my transform funcition is the same as the map function, except the map uses long and we need doubles
      temp = (i - ylo) * (gy - h - gy) / (yhi - ylo) + gy;
      if (i == 0)
      {
        d.drawFastHLine(gx - 3, temp, w + 3, SSD1306_WHITE);
      }
      else
      {
        d.drawFastHLine(gx - 3, temp, 3, SSD1306_WHITE);
      }
      d.setCursor(gx - 27, temp - 3);
      d.println(i, dig);
    }
    // draw x scale
    for (i = xlo; i <= xhi; i += xinc)
    {
      // compute the transform
      temp = (i - xlo) * (w) / (xhi - xlo) + gx;
      if (i == 0)
      {
        d.drawFastVLine(temp, gy - h, h + 3, SSD1306_WHITE);
      }
      else
      {
        d.drawFastVLine(temp, gy, 3, SSD1306_WHITE);
      }
      d.setCursor(temp, gy + 6);
      d.println(i, dig);
    }
  }

  // graph drawn now plot the data
  // the entire plotting code are these few lines...

  x = (x - xlo) * (w) / (xhi - xlo) + gx;
  y = (y - ylo) * (gy - h - gy) / (yhi - ylo) + gy;
  // d.drawCircle(ox + 2, oy + 1, 2, SSD1306_BLACK); //erase previous one
  d.fillRect(ox, 0, X_INC * 2, GRAPH_HEIGHT, SSD1306_BLACK);
  d.drawLine(ox, oy, x, y, SSD1306_WHITE);
  d.drawLine(ox, oy - 1, x, y - 1, SSD1306_WHITE);
  d.drawCircle(x + 2, y, 2, SSD1306_WHITE); // draw latest one
  ox = x;
  oy = y;

  // up until now print sends data to a video buffer NOT the screen
  // this call sends the data to the screen
  d.display();
}

/////////////////////////////////////////////////////////
