#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>

// ---------- WiFi AP config ----------
const char* ssid     = "CAR_AP";
const char* password = "12345678";  // min 8 chars

ESP8266WebServer server(80);

// ---------- Motor pins (L293 / L298) ----------
const int IN1 = D1;   // Right side dir1
const int IN2 = D2;   // Right side dir2
const int IN3 = D5;   // Left side dir1
const int IN4 = D6;   // Left side dir2

const int ENA = D7;   // PWM - right side enable
const int ENB = D8;   // PWM - left side enable

// ---------- Speed settings (0–1023) ----------
int speedRight = 900;  // adjust if needed
int speedLeft  = 900;

// ---------- Current command from joystick ----------
char currentCmd = 's'; // 'f','b','l','r','s'

// ---------- Motor functions ----------
void stopAll() {
  analogWrite(ENA, 0);
  analogWrite(ENB, 0);

  digitalWrite(IN1, LOW);
  digitalWrite(IN2, LOW);
  digitalWrite(IN3, LOW);
  digitalWrite(IN4, LOW);
}

void forward() {
  analogWrite(ENA, speedRight);
  analogWrite(ENB, speedLeft);

  digitalWrite(IN1, HIGH);
  digitalWrite(IN2, LOW);
  digitalWrite(IN3, HIGH);
  digitalWrite(IN4, LOW);
}

void backward() {
  analogWrite(ENA, speedRight);
  analogWrite(ENB, speedLeft);

  digitalWrite(IN1, LOW);
  digitalWrite(IN2, HIGH);
  digitalWrite(IN3, LOW);
  digitalWrite(IN4, HIGH);
}

void turnLeft() {
  // Right side forward, left side backward → spin left
  analogWrite(ENA, speedRight);
  analogWrite(ENB, speedLeft);

  digitalWrite(IN1, HIGH);
  digitalWrite(IN2, LOW);
  digitalWrite(IN3, LOW);
  digitalWrite(IN4, HIGH);
}

void turnRight() {
  // Left side forward, right side backward → spin right
  analogWrite(ENA, speedRight);
  analogWrite(ENB, speedLeft);

  digitalWrite(IN1, LOW);
  digitalWrite(IN2, HIGH);
  digitalWrite(IN3, HIGH);
  digitalWrite(IN4, LOW);
}

void applyCommand(char cmd) {
  switch (cmd) {
    case 'f': forward();  break;
    case 'b': backward(); break;
    case 'l': turnLeft(); break;
    case 'r': turnRight();break;
    default:  stopAll();  break;
  }
}

// ---------- HTTP handlers ----------
void handleRoot() {
  server.send(200, "text/plain", "ESP8266 car ready");
}

void handleCmd() {
  if (!server.hasArg("m")) {
    server.send(400, "text/plain", "Missing m param");
    return;
  }

  String m = server.arg("m");
  m.toLowerCase();

  if (m == "f" || m == "b" || m == "l" || m == "r" || m == "s") {
    currentCmd = m[0];
    Serial.print("Received cmd: ");
    Serial.println(currentCmd);
  } else {
    currentCmd = 's';
  }

  server.send(200, "text/plain", "OK");
}

void setup() {
  Serial.begin(115200);
  Serial.println();
  Serial.println("ESP8266 CAR BOOTING...");

  // Motor pins
  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);
  pinMode(IN3, OUTPUT);
  pinMode(IN4, OUTPUT);
  pinMode(ENA, OUTPUT);
  pinMode(ENB, OUTPUT);

  stopAll();
  delay(500);

  // WiFi AP setup
  WiFi.mode(WIFI_AP);
  bool apOk = WiFi.softAP(ssid, password);

  Serial.print("Starting AP: ");
  Serial.println(ssid);
  Serial.print("AP started: ");
  Serial.println(apOk ? "YES" : "NO");

  Serial.print("AP IP address: ");
  Serial.println(WiFi.softAPIP());  // typically 192.168.4.1

  // HTTP routes
  server.on("/", handleRoot);
  server.on("/cmd", handleCmd);
  server.begin();
  Serial.println("HTTP server started");
}

void loop() {
  server.handleClient();
  applyCommand(currentCmd);
}
