#include <SPI.h>
#include <MFRC522.h>
#include <WiFi.h>
#include <WebServer.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
 
#define BNP_LED 2
#define NCP_LED 4
#define JMT_LED 16
// ==== WiFi Credentials ====
const char* ssid = "aiub";
const char* password = "12345678";
 
// ==== RFID Setup ====
#define SS_PIN 5
#define RST_PIN 15
MFRC522 mfrc522(SS_PIN, RST_PIN);
 
// ==== Button pins ====
const int BTN_BNP = 13;
const int BTN_NCP = 14;
const int BTN_JMT = 27;
const int BTN_STOP = 26;
 
// ==== LCD ====
LiquidCrystal_I2C lcd(0x27, 16, 2);  // adjust 0x27 if needed
 
// ==== State ====
String currentUID = "";
bool votingAllowed = false;
bool votedThisCard = false;
bool votingStopped = false;
 
// ==== Vote counts ====
unsigned int votesBNP = 0;
unsigned int votesNCP = 0;
unsigned int votesJMT = 0;
 
// Already voted UIDs
String usedUIDs[100];
int usedCount = 0;
 
// Vote log
struct Vote {
  String voter;
  String candidate;
};
Vote voteLog[100];
int voteCount = 0;
 
// ==== Web Server ====
WebServer server(80);
 
// Convert UID to string
String uidToString(MFRC522::Uid uid) {
  String s = "";
  for (byte i = 0; i < uid.size; i++) {
    if (uid.uidByte[i] < 0x10) s += "0";
    s += String(uid.uidByte[i], HEX);
  }
  s.toUpperCase();
  return s;
}
 
bool uidUsed(String uid) {
  for (int i = 0; i < usedCount; i++) {
    if (usedUIDs[i] == uid) return true;
  }
  return false;
}
 
void markUIDUsed(String uid) {
  if (usedCount < 100) {
    usedUIDs[usedCount++] = uid;
  }
}
 
bool pressed(int pin) {
  if (digitalRead(pin) == LOW) {
    delay(30);
    if (digitalRead(pin) == LOW) {
      while (digitalRead(pin) == LOW)
        ;
      return true;
    }
  }
  return false;
}
 
// ==== LCD Message Function ====
void showLCD(String msg) {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(msg);
}
 
// ==== HTML Page ====
String htmlPage() {
  String page = F(
    "<!DOCTYPE html><html><head>"
    "<meta name='viewport' content='width=device-width, initial-scale=1'>"
    "<title>Voting Machine</title>"
    "<style>"
    "body { font-family: Arial, sans-serif; text-align: center; margin: 20px; background:#f4f4f9; }"
    "h1 { color: #333; }"
    ".score { font-size: 20px; margin: 10px; padding: 8px; border-radius: 10px; display: inline-block; min-width: 100px; background: #fff; box-shadow: 0 2px 6px rgba(0,0,0,0.2); }"
    ".winner { font-size: 24px; color: green; margin-top: 20px; font-weight: bold; }"
    "table { width: 80%; margin: 20px auto; border-collapse: collapse; background: #fff; box-shadow: 0 2px 6px rgba(0,0,0,0.2); }"
    "th, td { border: 1px solid #ccc; padding: 10px; text-align: center; }"
    "th { background: #333; color: #fff; }"
    "</style>"
    "</head><body>"
    "<h1>Live Voting Results</h1>"
    "<div class='score'>BNP: <span id='bnp'>0</span></div>"
    "<div class='score'>NCP: <span id='ncp'>0</span></div>"
    "<div class='score'>JMT: <span id='jmt'>0</span></div>"
    "<div class='winner' id='winner'></div>"
    "<h2>Vote History</h2>"
    "<table id='voteTable'>"
    "<tr><th>Voter</th><th>Candidate</th></tr>"
    "</table>"
    "<script>"
    "async function fetchData(){"
    "let res=await fetch('/results');"
    "let data=await res.json();"
    "document.getElementById('bnp').innerText=data.BNP;"
    "document.getElementById('ncp').innerText=data.NCP;"
    "document.getElementById('jmt').innerText=data.JMT;"
    "document.getElementById('winner').innerText=data.Winner;"
    "}"
    "async function fetchLog(){"
    "let res=await fetch('/log');"
    "let data=await res.json();"
    "let table=document.getElementById('voteTable');"
    "table.innerHTML='<tr><th>Voter</th><th>Candidate</th></tr>';"
    "data.forEach(row=>{"
    "let tr=document.createElement('tr');"
    "tr.innerHTML='<td>'+row.voter+'</td><td>'+row.candidate+'</td>';"
    "table.appendChild(tr);"
    "});"
    "}"
    "setInterval(()=>{fetchData();fetchLog();},2000);"
    "fetchData();fetchLog();"
    "</script>"
    "</body></html>");
  return page;
}
 
// ==== JSON Results API ====
void handleResults() {
  String winner = "";
  if (votingStopped) {
    if (votesBNP > votesNCP && votesBNP > votesJMT) winner = "Winner: BNP";
    else if (votesNCP > votesBNP && votesNCP > votesJMT) winner = "Winner: NCP";
    else if (votesJMT > votesBNP && votesJMT > votesNCP) winner = "Winner: JMT";
    else winner = "Tie";
  }
  String json = "{";
  json += "\"BNP\":" + String(votesBNP) + ",";
  json += "\"NCP\":" + String(votesNCP) + ",";
  json += "\"JMT\":" + String(votesJMT) + ",";
  json += "\"Winner\":\"" + winner + "\"";
  json += "}";
  server.send(200, "application/json", json);
}
 
// ==== JSON Log API ====
void handleLog() {
  String json = "[";
  for (int i = 0; i < voteCount; i++) {
    if (i > 0) json += ",";
    json += "{\"voter\":\"" + voteLog[i].voter + "\",";
    json += "\"candidate\":\"" + voteLog[i].candidate + "\"}";
  }
  json += "]";
  server.send(200, "application/json", json);
}
 
void setup() {
  pinMode(BNP_LED,OUTPUT);
  pinMode(NCP_LED,OUTPUT);
  pinMode(JMT_LED,OUTPUT);
  
  Serial.begin(115200);
  SPI.begin();
  mfrc522.PCD_Init();
 
  pinMode(BTN_BNP, INPUT_PULLUP);
  pinMode(BTN_NCP, INPUT_PULLUP);
  pinMode(BTN_JMT, INPUT_PULLUP);
  pinMode(BTN_STOP, INPUT_PULLUP);
 
  // LCD setup
  lcd.init();
  lcd.backlight();
  showLCD("Scan card...");
 
  // Connect WiFi
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nConnected!");
  Serial.print("ESP32 IP Address: ");
  Serial.println(WiFi.localIP());
 
  // Start Web Server
  server.on("/", []() {
    server.send(200, "text/html", htmlPage());
  });
  server.on("/results", handleResults);
  server.on("/log", handleLog);
  server.begin();
 
  Serial.println("==== Voting Started ====");
  Serial.println("Scan card to enter...");
}
 
void loop() {
  server.handleClient();
 
  // STOP button
  if (!votingStopped && pressed(BTN_STOP)) {
    Serial.println("==== Voting Complete ====");
    votingStopped = true;
 
    // Show winner on LCD
    String winner = "Tie";
    if (votesBNP > votesNCP && votesBNP > votesJMT) winner = "BNP";
    else if (votesNCP > votesBNP && votesNCP > votesJMT) winner = "NCP";
    else if (votesJMT > votesBNP && votesJMT > votesNCP) winner = "JMT";
    showLCD("Winner is " + winner);
  }
 
  if (votingStopped) return;
 
  // RFID check
  if (mfrc522.PICC_IsNewCardPresent() && mfrc522.PICC_ReadCardSerial()) {
    String uid = uidToString(mfrc522.uid);
 
    if (uidUsed(uid)) {
      Serial.print("Denied - UID ");
      Serial.println(uid);
      votingAllowed = false;
      currentUID = "";
      showLCD("Denied");
      delay(1000);
      showLCD("Scan card...");
    } else {
      Serial.print("Approved - UID ");
      Serial.println(uid);
      votingAllowed = true;
      votedThisCard = false;
      currentUID = uid;
      showLCD("Approved");
    }
 
    mfrc522.PICC_HaltA();
    mfrc522.PCD_StopCrypto1();
  }
 
  // Button voting
  if (votingAllowed && currentUID.length() > 0) {
    if (pressed(BTN_BNP)) {
      if (!votedThisCard) {
        votesBNP++;
        Serial.println("Voted BNP");
        voteLog[voteCount++] = { currentUID, "BNP" };
        markUIDUsed(currentUID);
        votedThisCard = true;
        votingAllowed = false;
        currentUID = "";
        digitalWrite(BNP_LED, HIGH);
        delay(1000);
        digitalWrite(BNP_LED, LOW);
        showLCD("Scan card...");
      }
    } else if (pressed(BTN_NCP)) {
      if (!votedThisCard) {
        votesNCP++;
        Serial.println("Voted NCP");
        voteLog[voteCount++] = { currentUID, "NCP" };
        markUIDUsed(currentUID);
        votedThisCard = true;
        votingAllowed = false;
        currentUID = "";
        digitalWrite(NCP_LED, HIGH);
        delay(1000);
        digitalWrite(NCP_LED, LOW);
        showLCD("Scan card...");
      }
    } else if (pressed(BTN_JMT)) {
      if (!votedThisCard) {
        votesJMT++;
        Serial.println("Voted JMT");
        voteLog[voteCount++] = { currentUID, "JMT" };
        markUIDUsed(currentUID);
        votedThisCard = true;
        votingAllowed = false;
        currentUID = "";
        digitalWrite(JMT_LED, HIGH);
        delay(1000);
        digitalWrite(JMT_LED, LOW);
        showLCD("Scan card...");
      }
    }
  }
}