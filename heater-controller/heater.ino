#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>

#include <OneWire.h>
#include <DallasTemperature.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
#define ONE_WIRE_BUS D5
#define RELAY D6
#define HIT_TOP 1
#define HIT_BOTTOM 2
#define SUHU_MAX 80.0
#define SUHU_MIN 60.0

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);

WiFiClient wifi;
HTTPClient http;

byte hitLast = HIT_BOTTOM;
boolean relayStatus = HIGH;

float suhu = 0;
float suhuLast = 0;
int counter = 0;

void setup() {
  Serial.begin(115200);
//  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(RELAY, OUTPUT);

  WiFi.mode(WIFI_STA);
  WiFi.begin("ssid", "password");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  sensors.begin();

  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { // Address 0x3D for 128x64
    Serial.println(F("SSD1306 allocation failed"));
    for(;;);
  }
  delay(2000);
  
  display.clearDisplay();
//  display.ssd1306_command(SSD1306_SETCONTRAST);
//  display.ssd1306_command(0x8F);
//  display.ssd1306_command(0x47);
//  display.dim(true);
  display.setTextSize(2);
  display.setTextColor(WHITE);

  sensors.requestTemperatures();
  suhu = sensors.getTempCByIndex(0);
  if (suhu > SUHU_MAX) {
    hitLast = HIT_TOP;
    relayStatus = LOW;
  }
}

void loop() {
  sensors.requestTemperatures();
  suhu = sensors.getTempCByIndex(0);

  if (suhu < 0) {
    delay(1000);
    return;
  }

  Serial.println(suhu);

  if (suhu != suhuLast) {
    suhuLast = suhu;

    display.clearDisplay();
    display.setCursor(0,0);
    display.println(suhu);
    display.display();
  }

  if (suhu > SUHU_MAX && hitLast == HIT_BOTTOM) {
    hitLast = HIT_TOP;
    relayStatus = LOW;
    Send(suhu, relayStatus);
  }
  else if (suhu < SUHU_MIN && hitLast == HIT_TOP) {
    hitLast = HIT_BOTTOM;
    relayStatus = HIGH;
    Send(suhu, relayStatus);
  }

  digitalWrite(RELAY, relayStatus);

  if (counter++ > 15) {
    Send(suhu, relayStatus);
    counter = 0;
  }

  delay(1000);
}

void Send(float suhu, boolean relay) {
  Serial.print("Sending.. ");
  String url = "http://api.thingspeak.com/update?api_key=SECRETAPIKEY";
  url.concat("&field1="+ String(suhu));
  url.concat("&field2="+ String(relay));
  if (http.begin(wifi,url)) {
    http.GET();
    http.end();
    Serial.println("OK");
  }
  else {
    Serial.println("Err?");
  }
}
