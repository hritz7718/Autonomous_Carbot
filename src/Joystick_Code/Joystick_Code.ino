#include <WiFi.h>
#include <HTTPClient.h>

// === WiFi of the CAR (ESP8266 AP) ===
const char* ssid     = "CAR_AP";
const char* password = "12345678";
const char* carIP    = "192.168.4.1";

// === Joystick pins (ESP32-S3) ===
const int VRX = 4;   // ADC
const int VRY = 5;   // ADC
const int SW  = 6;   // Digital

// Calibrated center values
int centerX = 2048;
int centerY = 2048;

// Dead zone around center
int dead = 600;

// Last command we sent
String lastCmd = "s";

void sendCmd(const String& cmd) {
  if (cmd == lastCmd) return;  // only send when it changes
  lastCmd = cmd;

  if (WiFi.status() != WL_CONNECTED) {
    Serial0.println("WiFi not connected");
    return;
  }

  HTTPClient http;
  String url = String("http://") + carIP + "/cmd?m=" + cmd;

  Serial0.print("Sending cmd: ");
  Serial0.println(url);

  http.begin(url);
  int httpCode = http.GET();
  http.end();

  Serial0.print("HTTP code: ");
  Serial0.println(httpCode);
}

// --- Calibrate joystick center at startup ---
void calibrateJoystick() {
  long sumX = 0;
  long sumY = 0;

  Serial0.println("Calibrating joystick... Don't touch it!");

  for (int i = 0; i < 100; i++) {
    sumX += analogRead(VRX);
    sumY += analogRead(VRY);
    delay(10);
  }

  centerX = sumX / 100;
  centerY = sumY / 100;

  Serial0.print("CenterX = "); Serial0.println(centerX);
  Serial0.print("CenterY = "); Serial0.println(centerY);
}

void setup() {
  Serial0.begin(115200);
  Serial0.println();
  Serial0.println("ESP32-S3 Joystick Controller starting...");

  pinMode(SW, INPUT_PULLUP);

  // Calibrate joystick center (stick untouched)
  calibrateJoystick();

  // WiFi connect to CAR_AP
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  Serial0.print("Connecting to ");
  Serial0.println(ssid);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial0.print(".");
  }
  Serial0.println();
  Serial0.print("Connected. IP: ");
  Serial0.println(WiFi.localIP());
}

void loop() {
  int x = analogRead(VRX);  // 0..4095
  int y = analogRead(VRY);
  int sw = digitalRead(SW);

  String cmd = "s";  // default is STOP

  // Debug print so you see what happens
  Serial0.print("x="); Serial0.print(x);
  Serial0.print(" y="); Serial0.print(y);
  Serial0.print(" centerX="); Serial0.print(centerX);
  Serial0.print(" centerY="); Serial0.print(centerY);
  Serial0.print(" sw="); Serial0.print(sw);

  // Decide command based on offset from center
  if (y < centerY - dead) {
    cmd = "f";             // push UP
  } else if (y > centerY + dead) {
    cmd = "b";             // push DOWN
  } else if (x > centerX + dead) {
    cmd = "r";             // push RIGHT
  } else if (x < centerX - dead) {
    cmd = "l";             // push LEFT
  } else {
    cmd = "s";             // in the dead zone → STOP
  }

  Serial0.print("  cmd=");
  Serial0.println(cmd);

  sendCmd(cmd);
  delay(100);
}
