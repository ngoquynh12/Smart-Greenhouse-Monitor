#include <ESP8266WiFi.h>
#include <FirebaseESP8266.h>
#include <EEPROM.h>

// WiFi mặc định
#define DEFAULT_SSID "MinisDora"
#define DEFAULT_PASSWORD "quynh123"

// Firebase config
#define FIREBASE_HOST "smart-greenhouse-monitor-dc6fa-default-rtdb.firebaseio.com"
#define FIREBASE_AUTH "CzaOIxdbQIhL00rqCt0QK29SrUxfTwSbkCsG25Ji"
#define FIREBASE_URL "https://smart-greenhouse-monitor-dc6fa-default-rtdb.firebaseio.com"

// EEPROM
#define EEPROM_SIZE 512
#define EEPROM_SSID_ADDR 0
#define EEPROM_PASS_ADDR 100

FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;

String ssid = "";
String password = "";
String currentLine = "";

unsigned long lastSend = 0;
bool triedFallback = false;
String lastMode = ""; //Dùng để tránh gửi trùng mode

void saveWiFiToEEPROM(String s, String p) {
  EEPROM.begin(EEPROM_SIZE);
  for (int i = 0; i < s.length(); i++) EEPROM.write(EEPROM_SSID_ADDR + i, s[i]);
  EEPROM.write(EEPROM_SSID_ADDR + s.length(), '\0');
  for (int i = 0; i < p.length(); i++) EEPROM.write(EEPROM_PASS_ADDR + i, p[i]);
  EEPROM.write(EEPROM_PASS_ADDR + p.length(), '\0');
  EEPROM.commit();
}

void readWiFiFromEEPROM(String &s, String &p) {
  char ssidBuf[100], passBuf[100];
  int i = 0, ch;
  EEPROM.begin(EEPROM_SIZE);
  while ((ch = EEPROM.read(EEPROM_SSID_ADDR + i)) != '\0' && i < 99) ssidBuf[i++] = ch;
  ssidBuf[i] = '\0';
  i = 0;
  while ((ch = EEPROM.read(EEPROM_PASS_ADDR + i)) != '\0' && i < 99) passBuf[i++] = ch;
  passBuf[i] = '\0';
  s = String(ssidBuf);
  p = String(passBuf);
}

void connectWiFi(String ssid, String password) {
  WiFi.disconnect();
  delay(500);
  WiFi.begin(ssid.c_str(), password.c_str());
  int retry = 0;
  while (WiFi.status() != WL_CONNECTED && retry++ < 20) {
    delay(500);
    Serial.print(".");
  }
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\n✅ Đã kết nối WiFi!");
    Serial.println("📶 Đã đăng nhập vào mạng: " + WiFi.SSID());
    Serial.println("🌐 IP: " + WiFi.localIP().toString());
  } else {
    Serial.println("\n❌ Không kết nối được WiFi: " + ssid);
  }
}

void setup() {
  Serial.begin(115200);
  delay(1000);

  readWiFiFromEEPROM(ssid, password);

  if (ssid.length() > 0 && password.length() > 0) {
    Serial.println("📚 Đọc WiFi từ EEPROM:");
    Serial.println("SSID: " + ssid);
    Serial.println("Password: " + password);
  } else {
    Serial.println("⚠️ EEPROM trống, dùng WiFi mặc định.");
    ssid = DEFAULT_SSID;
    password = DEFAULT_PASSWORD;
  }

  Serial.println("🔌 Đang kết nối WiFi...");
  connectWiFi(ssid, password);

  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("🔁 Fallback WiFi mặc định...");
    connectWiFi(DEFAULT_SSID, DEFAULT_PASSWORD);
    ssid = DEFAULT_SSID;
    password = DEFAULT_PASSWORD;
  }

  config.database_url = FIREBASE_URL;
  config.signer.tokens.legacy_token = FIREBASE_AUTH;
  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);

  Serial.println("🔥 Firebase sẵn sàng");
  delay(2000);

  bool ssidOk = false, passOk = false;
  String newSSID, newPass;

  if (Firebase.getString(fbdo, "/wifi/ssid")) {
    newSSID = fbdo.stringData();
    Serial.println("📶 Đọc SSID mới từ Firebase: " + newSSID);
    ssidOk = true;
  } else {
    Serial.println("❌ Không đọc được SSID: " + fbdo.errorReason());
  }

  if (Firebase.getString(fbdo, "/wifi/password")) {
    newPass = fbdo.stringData();
    Serial.println("🔑 Đọc Password mới từ Firebase: " + newPass);
    passOk = true;
  } else {
    Serial.println("❌ Không đọc được Password: " + fbdo.errorReason());
  }

  if (ssidOk && passOk) {
    saveWiFiToEEPROM(newSSID, newPass);
    Serial.println("💾 Đã lưu WiFi mới vào EEPROM.");
    Serial.println("🔄 Đang kết nối WiFi mới...");
    connectWiFi(newSSID, newPass);

    if (WiFi.status() == WL_CONNECTED)
      Firebase.setString(fbdo, "/wifi/wifi_status", "connected");
    else
      Firebase.setString(fbdo, "/wifi/wifi_status", "disconnected");
  }
}

void loop() {
  static String input = "";
  float temperature, humidity, soil, light;

  if (WiFi.status() != WL_CONNECTED && !triedFallback) {
    Serial.println("\n⚠️ WiFi mất kết nối, fallback WiFi mặc định...");
    connectWiFi(DEFAULT_SSID, DEFAULT_PASSWORD);
    triedFallback = true;
  }

  while (Serial.available()) {
    char c = Serial.read();
    if (c == '\n' || c == '\r') {
      input.trim();
      if (input.startsWith("{") && input.endsWith("}")) {
        Serial.print("📥 JSON nhận được: ");
        Serial.println(input);
        FirebaseJson json;
        json.setJsonData(input);
        Firebase.setJSON(fbdo, "/sensors", json);
        Serial.println("✅ Đã gửi JSON lên Firebase!");
      } else {
        Serial.println("❌ Không phải JSON hợp lệ: " + input);
      }
      input = "";
    } else {
      input += c;
    }
  }
  // Gửi trạng thái FAN/PUMP
  if (millis() - lastSend > 2000) {
    lastSend = millis();
    String fan = "off", pump = "off";
    if (Firebase.getString(fbdo, "/control/fan")) fan = fbdo.stringData();
    if (Firebase.getString(fbdo, "/control/pump")) pump = fbdo.stringData();
    char uartBuf[64];
    snprintf(uartBuf, sizeof(uartBuf), "FAN:%s;PUMP:%s\n", fan.c_str(), pump.c_str());
    Serial.print(uartBuf);

    // Gửi MODE nếu thay đổi
    String mode = "manual";
    if (Firebase.getString(fbdo, "/control/mode")) {
      mode = fbdo.stringData();
      if (mode != lastMode) {
        lastMode = mode;
        String modeCmd = "MODE:" + mode + "\n";
        Serial.print(modeCmd);
      }
    }
  }
  delay(500);
}
