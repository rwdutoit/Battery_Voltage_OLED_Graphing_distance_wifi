/*

  This program provides functions to draw:
  1. horizontal bar graph
  2. vertical bar graph
  3. dial
  4. cartesian graph

  It requires an microcrontrioller Arduino, teensy and a 0.96 dispaly (1306 driver chip)

  display. sources
  https://www.amazon.com/Diymall-Yellow-Arduino-display.-Raspberry/dp/B00O2LLT30/ref=pd_lpo_vtph_147_bs_t_1?_encoding=UTF8&psc=1&refRID=BQAPE07SVJQNDKEVMTQZ
  https://www.amazon.com/dp/B06XRCQZRX/ref=sspa_dk_detail_0?psc=1&pd_rd_i=B06XRCQZRX&pd_rd_wg=aGuhK&pd_rd_r=1WJPXAG68XFSADGDPNZR&pd_rd_w=DAAsB

  Adafruit libraries
  https://github.com/adafruit/Adafruit-GFX-Library/archive/master.zip
  https://github.com/adafruit/Adafruit_SSD1306

  Revisions
  rev     date        author      description
  1       12-5-2017   kasprzak    initial creation


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

#include <Ethernet.h>
#include <PubSubClient.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Wire.h>
//Wifi ultrasonic reading using nodemcu
//made by Dhananjay
//cantact adhanu99.blogspot.com

#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>

// Replace with your network credentials
const char *ssid = "du_Toit";
const char *password = "Master101!";
const char *mqtt_server = "192.168.0.117";

const int lcdSda = D1;
const int lcdScl = D2;
const int trigPin = D5;
const int echoPin = D6;
const int buzzerPos = D7;
const int buzzerNeg = D8;

// defines variables
long duration;
double distance;

ESP8266WebServer server(80); //instantiate server at port 80 (http port)

WiFiClient espClient;
PubSubClient client(espClient);
IPAddress remote_mqtt_addr(192, 168, 0, 117);
unsigned long lastMsg = 0;
unsigned long lastMsgDistance = 0;
#define MSG_BUFFER_SIZE (5)
char msg[MSG_BUFFER_SIZE];
int value = 0;

String HTMLpage = "";

#define READ_PIN A0
#define OLED_RESET LED_BUILTIN
#define LOGO16_GLCD_HEIGHT 16
#define LOGO16_GLCD_WIDTH 16

// create what ever variables you need
double volts;
double bvolts;
double x, y;

// these are a required variables for the graphing functions
bool Redraw1 = true;
bool Redraw2 = true;
bool Redraw3 = true;
bool Redraw4 = true;
double ox, oy;

// create the display object
//Adafruit_SSD1306 display(OLED_RESET);
#define SCREEN_WIDTH 128   // OLED display width, in pixels
#define SCREEN_X_OFFSET 10 // OLED display width, in pixels
#define GRAPH_WIDTH (SCREEN_WIDTH - SCREEN_X_OFFSET)
#define X_INC 10
#define SCREEN_HEIGHT 64  // OLED display height, in pixels
#define SCREEN_Y_OFFSET 2 // OLED display width, in pixels
#define GRAPH_HEIGHT (SCREEN_HEIGHT - SCREEN_Y_OFFSET)

#define DISTANCE_MAX 5.400

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
    // Create a random client ID
    String clientId = "ESP8266Client-";
    clientId += String(random(0xffff), HEX);
    // Attempt to connect
    if (client.connect(clientId.c_str()))
    {
      Serial.println("connected");
      // Once connected, publish an announcement...
      snprintf(msg, MSG_BUFFER_SIZE, "%d", 0);
      client.publish("mqtt/distance1", "0");
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
  server.on("/", []() {
    HTMLpage = "<h1>Sensor to Node MCU Web Server</h1><h3>ultrasonic distance:</h3> <h1>" + String(distance) + "mm</14><head><META HTTP-EQUIV=\"refresh\" CONTENT=\"1\"></head>";
    server.send(200, "text/html", HTMLpage);
  });

  server.begin();
  Serial.println("Web server started!");
}

void setupDistanceWifi(void)
{

  pinMode(trigPin, OUTPUT);   // Sets the trigPin as an Output
  pinMode(buzzerPos, OUTPUT); // Sets the trigPin as an Output
  pinMode(buzzerNeg, OUTPUT); // Sets the trigPin as an Output
  digitalWrite(buzzerNeg, LOW);
  pinMode(echoPin, INPUT);

  delay(1000);
}

void setupMqtt()
{
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
}

void setup()
{

  Serial.begin(115200);
  setup_wifi();

  // initialize the display
  // note you may have to change the address
  // the most common are 0X3C and 0X3D

  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  // if you distribute your code, the adafruit license requires you show their splash screen
  // otherwise clear the video buffer memory then display
  display.display();
  //delay(2000);
  display.clearDisplay();
  display.display();

  setupDistanceWifi();
  // establish whatever pin reads you need
  pinMode(A0, INPUT);
  setupMqtt();
}

void loopMqtt()
{
  if (!client.connected())
  {
    reconnect();
  }
  client.loop();

  unsigned long now = millis();
  if (now - lastMsg > 1000)
  {
    lastMsg = now;
    snprintf(msg, MSG_BUFFER_SIZE, "%d", distance);
    client.publish("mqtt/distance1", msg);
    Serial.print("published");
  }
}

void loopDistance()
{
  unsigned long now = millis();
  if (now - lastMsgDistance > 1000)
  {
    lastMsgDistance = now;

    // read some values and convert to volts
    bvolts = analogRead(A0);
    volts = (bvolts / 204.6);

    // Clears the trigPin
    digitalWrite(trigPin, LOW);
    delayMicroseconds(2);
    // Sets the trigPin on HIGH state for 10 micro seconds
    digitalWrite(trigPin, HIGH);
    delayMicroseconds(10);
    digitalWrite(trigPin, LOW);
    // Reads the echoPin, returns the sound wave travel time in microseconds
    duration = pulseIn(echoPin, HIGH);
    // Calculating the distance
    distance = duration * 0.034 / 2;
    // Prints the distance on the Serial Monitor
    Serial.print("Distance: ");
    Serial.println(distance);
    if (distance < 10)
    {
      digitalWrite(buzzerPos, HIGH);
    }
    else
    {
      digitalWrite(buzzerPos, LOW);
    }

    //    DrawCGraph(display, x++, bvolts, 30, 50, 75, 30, 0, 100, 25, 0, 6000, 512, 0, "cm vs Seconds", Redraw4);
    x = x + 5;
    DrawCGraph(display, x++, distance / 1000, SCREEN_X_OFFSET, GRAPH_HEIGHT, GRAPH_WIDTH, GRAPH_HEIGHT, 0, GRAPH_WIDTH, X_INC, 0, DISTANCE_MAX, 1, 0, "m vs Seconds", Redraw4);
    if (x >= GRAPH_WIDTH)
    {
      x = 0;
      Redraw4 = true;
    }
  }
}

unsigned long OldTime;
unsigned long counter;
void loop(void)
{
  loopDistance();
  loopMqtt();
  server.handleClient();
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
  d.fillRect(ox, 0, X_INC*2, GRAPH_HEIGHT, SSD1306_BLACK);
  d.drawLine(ox, oy, x, y, SSD1306_WHITE);
  d.drawLine(ox, oy - 1, x, y - 1, SSD1306_WHITE);
  d.drawCircle(x + 2, y + 1, 2, SSD1306_WHITE); //draw latest one
  ox = x;
  oy = y;

  // up until now print sends data to a video buffer NOT the screen
  // this call sends the data to the screen
  d.display();
}

/////////////////////////////////////////////////////////
