#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include <time.h>
#include <TZ.h>
// #include <FS.h>
// #include <LittleFS.h>
// #include <CertStoreBearSSL.h>
#include <ArduinoJson.h>
#include <Adafruit_Fingerprint.h>

// #include <HardwareSerial.h>
#include <SoftwareSerial.h>

// Menginisialisasi komunikasi serial untuk sensor sidik jari menggunakan SoftwareSerial di pin 13 (RX) dan 15 (TX).
SoftwareSerial mySerial(13, 15);
Adafruit_Fingerprint finger = Adafruit_Fingerprint(&mySerial);
  
uint8_t id = 0;           // Variabel untuk menyimpan ID sidik jari yang terdeteksi.
const int relay_dl = 12;  // Pin yang digunakan untuk mengendalikan relay brankas.
const int ledPin_merah = 9;
const int ledPin_hijau = 16;
const int buzzer = 4;
const int getar = 14;
bool ledState = LOW;

// Update these with values suitable for your network.
const char *ssid = "DLB94859";
const char *password = "12345678";

// IPAddress local_IP(192, 168, 43, 15);
// IPAddress subnet(255, 255, 255, 0);
// IPAddress gateway(192, 168, 43, 1);
// IPAddress primaryDNS(8, 8, 8, 8);

const char *mqtt_server = "cbaf6706df604b55adafeef2e822ffa6.s1.eu.hivemq.cloud";
const int mqtt_port = 8883;

const char *apiKey = "MzBlZGFkNTUtM2IyZS00ODA1LTg2N2MtZDZlNzVlNjc1MzAy";  // Ganti dengan kunci API OneSignal Anda
const char *appID = "49b0050a-99f4-4842-ba5b-db1b516fb6cb";

const char *mqtt_username = "Perangkat1";
const char *mqtt_password = "Perangkat123@";

// Variabel global untuk menyimpan topicMobile
String topicResp = "response-mobile-";
String topicMobile = "";

//ID Perangkat & PIN
const char *id_perangkat_default = "84:f3:eb:20:90:94";
const char *pin_perangkat_default = "8765";

//Inisialisasi variabel untuk menghitung upaya fingerprint yang salah
int wrongAttempts = 0;
int maxWrongAttempts = 7;

bool isEnroll = false;  //apakah fingerprint digunakan untuk mendaftar

bool isFingerActive = true;        //apakah fingerprint aktiv
bool isVibrateActive = true;       //apakah getaran aktiv
bool isNotificationActive = true;  //apakah getaran aktiv

bool isCameraActive = true;  //apakah kamera aktiv
bool faceTrue = false;       //apakah kamera aktiv

StaticJsonDocument<200> doc;

static const char *root_ca PROGMEM = R"EOF(
-----BEGIN CERTIFICATE-----
MIIFazCCA1OgAwIBAgIRAIIQz7DSQONZRGPgu2OCiwAwDQYJKoZIhvcNAQELBQAw
TzELMAkGA1UEBhMCVVMxKTAnBgNVBAoTIEludGVybmV0IFNlY3VyaXR5IFJlc2Vh
cmNoIEdyb3VwMRUwEwYDVQQDEwxJU1JHIFJvb3QgWDEwHhcNMTUwNjA0MTEwNDM4
WhcNMzUwNjA0MTEwNDM4WjBPMQswCQYDVQQGEwJVUzEpMCcGA1UEChMgSW50ZXJu
ZXQgU2VjdXJpdHkgUmVzZWFyY2ggR3JvdXAxFTATBgNVBAMTDElTUkcgUm9vdCBY
MTCCAiIwDQYJKoZIhvcNAQEBBQADggIPADCCAgoCggIBAK3oJHP0FDfzm54rVygc
h77ct984kIxuPOZXoHj3dcKi/vVqbvYATyjb3miGbESTtrFj/RQSa78f0uoxmyF+
0TM8ukj13Xnfs7j/EvEhmkvBioZxaUpmZmyPfjxwv60pIgbz5MDmgK7iS4+3mX6U
A5/TR5d8mUgjU+g4rk8Kb4Mu0UlXjIB0ttov0DiNewNwIRt18jA8+o+u3dpjq+sW
T8KOEUt+zwvo/7V3LvSye0rgTBIlDHCNAymg4VMk7BPZ7hm/ELNKjD+Jo2FR3qyH
B5T0Y3HsLuJvW5iB4YlcNHlsdu87kGJ55tukmi8mxdAQ4Q7e2RCOFvu396j3x+UC
B5iPNgiV5+I3lg02dZ77DnKxHZu8A/lJBdiB3QW0KtZB6awBdpUKD9jf1b0SHzUv
KBds0pjBqAlkd25HN7rOrFleaJ1/ctaJxQZBKT5ZPt0m9STJEadao0xAH0ahmbWn
OlFuhjuefXKnEgV4We0+UXgVCwOPjdAvBbI+e0ocS3MFEvzG6uBQE3xDk3SzynTn
jh8BCNAw1FtxNrQHusEwMFxIt4I7mKZ9YIqioymCzLq9gwQbooMDQaHWBfEbwrbw
qHyGO0aoSCqI3Haadr8faqU9GY/rOPNk3sgrDQoo//fb4hVC1CLQJ13hef4Y53CI
rU7m2Ys6xt0nUW7/vGT1M0NPAgMBAAGjQjBAMA4GA1UdDwEB/wQEAwIBBjAPBgNV
HRMBAf8EBTADAQH/MB0GA1UdDgQWBBR5tFnme7bl5AFzgAiIyBpY9umbbjANBgkq
hkiG9w0BAQsFAAOCAgEAVR9YqbyyqFDQDLHYGmkgJykIrGF1XIpu+ILlaS/V9lZL
ubhzEFnTIZd+50xx+7LSYK05qAvqFyFWhfFQDlnrzuBZ6brJFe+GnY+EgPbk6ZGQ
3BebYhtF8GaV0nxvwuo77x/Py9auJ/GpsMiu/X1+mvoiBOv/2X/qkSsisRcOj/KK
NFtY2PwByVS5uCbMiogziUwthDyC3+6WVwW6LLv3xLfHTjuCvjHIInNzktHCgKQ5
ORAzI4JMPJ+GslWYHb4phowim57iaztXOoJwTdwJx4nLCgdNbOhdjsnvzqvHu7Ur
TkXWStAmzOVyyghqpZXjFaH3pO3JLF+l+/+sKAIuvtd7u+Nxe5AW0wdeRlN8NwdC
jNPElpzVmbUq4JUagEiuTDkHzsxHpFKVK7q4+63SM1N95R1NbdWhscdCb+ZAJzVc
oyi3B43njTOQ5yOf+1CceWxG1bQVs5ZufpsMljq4Ui0/1lvh+wjChP4kqKOJ2qxq
4RgqsahDYVvTH9w7jXbyLeiNdd8XM2w9U/t7y0Ff/9yi0GE44Za4rF2LN9d11TPA
mRGunUHBcnWEvgJBQl9nJEiU0Zsnvgc/ubhPgXRR4Xq37Z0j4r7g1SgEEzwxA57d
emyPxgcYxn/eR44/KJ4EBs+lVDR3veyJm+kXQ99b21/+jh5Xos1AnX5iItreGCc=
-----END CERTIFICATE-----
)EOF";

X509List cert(root_ca);
WiFiClientSecure espClient;
PubSubClient client(espClient);

unsigned long lastMsg = 0;
#define MSG_BUFFER_SIZE (500)
char msg[MSG_BUFFER_SIZE];
int value = 0;

unsigned long previousMillis = 0;
const long interval = 3000;  // Waktu dalam milidetik (3 detik)

void setup_wifi() {
  delay(10);
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  // WiFi.config(local_IP, gateway, subnet, primaryDNS);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(200);
    Serial.print(".");
  }

  randomSeed(micros());

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void setup() {
  Serial.begin(115200);
  espClient.setInsecure();
  delay(500);

  pinMode(ledPin_merah, OUTPUT);
  pinMode(ledPin_hijau, OUTPUT);
  pinMode(buzzer, OUTPUT);
  pinMode(relay_dl, OUTPUT);
  pinMode(getar, INPUT);

  setup_wifi();
  setDateTime();

  pinMode(LED_BUILTIN, OUTPUT);  // Initialize the LED_BUILTIN pin as an output

  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(callback);

  // Menginisialisasi sensor sidik jari.
  finger.begin(57600);  // Kecepatan komunikasi dengan sensor sidik jari.
  delay(250);

  if (finger.verifyPassword()) {
    Serial.println("Found fingerprint sensor!");  // Sensor sidik jari berhasil terdeteksi.
  } else {
    Serial.println("Did not find fingerprint sensor :(");  // Sensor sidik jari tidak ditemukan.
    while (1) {
      delay(1);
    }
  }

  finger.getTemplateCount();  // Mendapatkan jumlah templat sidik jari yang tersimpan.
  Serial.print("Sensor contains ");
  Serial.print(finger.templateCount);
  Serial.println(" templates");

  delay(250);

  digitalWrite(ledPin_hijau, LOW);
  digitalWrite(ledPin_merah, HIGH);
  digitalWrite(relay_dl, HIGH);
}

void sendNotification() {
  delay(50);

  doc["notification"] = "true";

  String jsonString = "";
  serializeJson(doc, jsonString);

  client.publish("notification-topic", jsonString.c_str());
  Serial.println("Status Dikirim.");
}

void setDateTime() {
  // You can use your own timezone, but the exact time is not used at all.
  // Only the date is needed for validating the certificates.
  configTime(TZ_Asia_Jakarta, "pool.ntp.org", "time.nist.gov");

  Serial.print("Waiting for NTP time sync: ");
  time_t now = time(nullptr);
  while (now < 8 * 3600 * 2) {
    delay(100);
    Serial.print(".");
    now = time(nullptr);
  }
  Serial.println();

  struct tm timeinfo;
  gmtime_r(&now, &timeinfo);
  Serial.printf("%s %s", tzname[0], asctime(&timeinfo));
}

void SendDeviceStatus() {
  delay(250);

  doc["type"] = "device-status";
  doc["gembok"] = digitalRead(relay_dl) == HIGH;  // True = Tertutup | False = Terbuka
  doc["finger"] = isFingerActive;
  doc["vibrate"] = isVibrateActive;
  doc["notification"] = isNotificationActive;
  doc["camera"] = isCameraActive;

  String jsonString = "";
  serializeJson(doc, jsonString);

  client.publish(topicMobile.c_str(), jsonString.c_str());
  Serial.println("Status Dikirim.");
}

void callback(char *topic, byte *payload, unsigned int length) {
  // Buat variabel String untuk menampung pesan JSON
  String message = "";

  for (int i = 0; i < length; i++) {
    message += (char)payload[i];
  }

  //Mengurai pesan JSON
  DeserializationError error = deserializeJson(doc, message);

  if (error) {
    Serial.print(F("deserializeJson() failed: "));
    Serial.print(message);
    Serial.println(error.f_str());
    return;
  }

  const char *id_perangkat_request_const = doc["id_perangkat"];
  const char *pin_perangkat_request_const = doc["pin_perangkat"];
  const char *command_request_const = doc["command"];

  const char *id_mobile_request_const = doc["id_mobile"];
  String id_mobile_request(id_mobile_request_const);

  String id_perangkat_request(id_perangkat_request_const);
  String pin_perangkat_request(pin_perangkat_request_const);
  String command_request(command_request_const);

  delay(50);

  topicMobile = topicResp + id_mobile_request.c_str();

  //Authentikasi

  if (command_request == "authentication") {
    if (id_perangkat_request == id_perangkat_default && pin_perangkat_request == pin_perangkat_default) {

      doc["type"] = "auth";
      doc["status"] = true;
      doc["message"] = "Berhasil masuk perangkat";

      String jsonString = "";
      serializeJson(doc, jsonString);

      client.publish(topicMobile.c_str(), jsonString.c_str());

    } else {

      doc["type"] = "auth";
      doc["status"] = false;
      doc["message"] = "Perangkat tidak ditemukan, periksa ID dan Pin";

      String jsonString = "";
      serializeJson(doc, jsonString);

      client.publish(topicMobile.c_str(), jsonString.c_str());
    }
  }

  //Rekam Sidik Jari

  if (command_request == "/rekam-sidik") {
    id++;
    isFingerActive = false;
    delay(250);
    isEnroll = true;

    Serial.print("Enrolling ID #");
    Serial.println(id);

    doc["type"] = "fingerprint-set-loading";
    doc["status"] = true;

    String jsonString = "";
    serializeJson(doc, jsonString);

    client.publish(topicMobile.c_str(), jsonString.c_str());

    Serial.println("Ready to enroll a fingerprint!");

    if (id == 0) {  // ID #0 not allowed, try again!
      isFingerActive = true;
      delay(250);
      isEnroll = false;
      doc["type"] = "fingerprint-set";
      doc["status"] = false;
      doc["message"] = "Gagal mendaftarkan sidik jari dengan ID: ";

      String jsonString = "";
      serializeJson(doc, jsonString);

      client.publish(topicMobile.c_str(), jsonString.c_str());
      return;
    }
  }

  //Perangkat

  if (command_request == "/matikan-alat") {
    Serial.println("Matikan alat.");

    //Mematikan
    isFingerActive = false;
    isVibrateActive = false;
    isNotificationActive = false;

    digitalWrite(ledPin_merah, LOW);
    digitalWrite(ledPin_hijau, LOW);
    //    digitalWrite(buzzer, LOW);
    digitalWrite(relay_dl, HIGH);

    SendDeviceStatus();
  }

  if (command_request == "/hidupkan-alat") {
    Serial.println("Hidupkan alat.");

    //Menghidupkan
    isFingerActive = true;
    isVibrateActive = true;
    isNotificationActive = true;

    SendDeviceStatus();
  }

  //Finger

  if (command_request == "/hidupkan-finger") {
    Serial.println("Hidupkan sensor finger.");

    //Menghidupkan
    isFingerActive = true;

    SendDeviceStatus();
  }

  if (command_request == "/matikan-finger") {
    Serial.println("Matikan sensor finger.");

    //Mematikan
    isFingerActive = false;

    SendDeviceStatus();
  }

  //Kamera

  if (command_request == "/hidupkan-kamera") {
    Serial.println("Hidupkan sensor kamera.");

    //Menghidupkan
    isCameraActive = true;

    SendDeviceStatus();
  }

  if (command_request == "/matikan-kamera") {
    Serial.println("Matikan sensor kamera.");

    //Mematikan
    isCameraActive = false;

    SendDeviceStatus();
  }

  //Getar

  if (command_request == "/hidupkan-getar") {
    Serial.println("Hidupkan sensor getaran.");

    //Menghidupkan
    isVibrateActive = true;

    SendDeviceStatus();
  }

  if (command_request == "/matikan-getar") {
    Serial.println("Hidupkan sensor getaran.");

    //Mematikan
    isVibrateActive = false;

    SendDeviceStatus();
  }

  //Notifiaksi

  if (command_request == "/hidupkan-notifikasi") {
    Serial.println("Hidupkan notifikasi.");

    //Menghidupkan
    isNotificationActive = true;

    SendDeviceStatus();
  }

  if (command_request == "/matikan-notifikasi") {
    Serial.println("Matikan notifikasi.");

    //Mematikan
    isNotificationActive = false;

    SendDeviceStatus();
  }

  if (command_request == "/tutup-gembok") {
    Serial.println("Tutup gembok.");
    digitalWrite(ledPin_merah, HIGH);
    digitalWrite(ledPin_hijau, LOW);
    digitalWrite(relay_dl, HIGH);

    SendDeviceStatus();
  }

  //Buzzer
  if (command_request == "/matikan-buzzer") {
    Serial.println("Buzzer Dimatikan.");
    digitalWrite(buzzer, LOW);
  }

  if (command_request == "/status") {
    SendDeviceStatus();
  }

  if (command_request == "/delete-finger") {
    for (int id = 1; id <= 50; id++) {
      if (finger.deleteModel(id) == FINGERPRINT_OK) {
        Serial.print("Menghapus sidik jari ID #");
        Serial.println(id);
      } else {
        Serial.print("Gagal menghapus sidik jari ID #");
        Serial.println(id);
      }
    }
  }
}

void reconnect() {
  // Loop until we’re reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection…");
    String clientId = "ESP8266Client - MyClient";
    // Attempt to connect
    // Insert your password
    if (client.connect(clientId.c_str(), mqtt_username, mqtt_password)) {
      Serial.println("mqtt connected");
      client.subscribe("request-mobile");
    } else {
      Serial.print("failed, rc = ");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

uint8_t getFingerprintID() {

  uint8_t p = finger.getImage();
  if (p != FINGERPRINT_OK) return -1;

  p = finger.image2Tz();
  if (p != FINGERPRINT_OK) return -1;

  p = finger.fingerFastSearch();
  if (p != FINGERPRINT_OK) {

    delay(250);

    digitalWrite(ledPin_hijau, LOW);   // Matikan LED hijau.
    digitalWrite(ledPin_merah, HIGH);  // Nyalakan LED merah.
    digitalWrite(relay_dl, HIGH);      // Tutup gembok
    SendDeviceStatus();

    wrongAttempts++;
    Serial.print("WrongAttemps: ");
    Serial.println(wrongAttempts);

    if (wrongAttempts >= maxWrongAttempts) {
      wrongAttempts = 0;
      digitalWrite(buzzer, HIGH);  // Nyalakan buzzer.
      Serial.println("Ada Penyusup!");

      if (isNotificationActive) {
        sendNotification();
      }
    }

    return -1;
  } else {
    wrongAttempts = 0;
    if (isCameraActive) {

      if (faceTrue) {
        // found a match!
        Serial.print("Found ID #");
        Serial.print(finger.fingerID);

        delay(250);

        digitalWrite(relay_dl, LOW);       // Buka gembok
        digitalWrite(ledPin_hijau, HIGH);  // Nyalakan LED hijau.
        digitalWrite(buzzer, LOW);         //matikan buzzer
        digitalWrite(ledPin_merah, LOW);   // Matikan LED merah.
        SendDeviceStatus();
      } else {
        digitalWrite(buzzer, HIGH);  // Nyalakan buzzer.
        Serial.println("Ada Penyusup, wajah tidak dikenali!");

        if (isNotificationActive) {
          sendNotification();
        }
      }

    } else {
      // found a match!
      Serial.print("Found ID #");
      Serial.print(finger.fingerID);

      delay(250);

      digitalWrite(relay_dl, LOW);       // Buka gembok
      digitalWrite(ledPin_hijau, HIGH);  // Nyalakan LED hijau.
      digitalWrite(buzzer, LOW);         //matikan buzzer
      digitalWrite(ledPin_merah, LOW);   // Matikan LED merah.
      SendDeviceStatus();
    }
  }

  return finger.fingerID;
}

uint8_t getFingerprintEnroll() {
  int p = -1;  // Variabel status operasi
  Serial.print("Waiting for a valid finger to enroll as #");
  Serial.println(id);

  // Menunggu hingga sidik jari yang valid ditemukan
  while (p != FINGERPRINT_OK) {
    p = finger.getImage();
    if (p == FINGERPRINT_OK) {
      Serial.println("Image taken");
    } else if (p == FINGERPRINT_NOFINGER) {
      Serial.println(".");
    } else if (p == FINGERPRINT_PACKETRECIEVEERR) {
      Serial.println("Communication error");
    } else if (p == FINGERPRINT_IMAGEFAIL) {
      Serial.println("Imaging error");
    } else {
      Serial.println("Unknown error");
    }
  }

  // Sidik jari yang valid telah diambil

  p = finger.image2Tz(1);
  if (p == FINGERPRINT_OK) {
    Serial.println("Image converted");
  } else if (p == FINGERPRINT_IMAGEMESS) {
    Serial.println("Image too messy");
    return false;
  } else if (p == FINGERPRINT_PACKETRECIEVEERR) {
    Serial.println("Communication error");
    return false;
  } else if (p == FINGERPRINT_FEATUREFAIL || p == FINGERPRINT_INVALIDIMAGE) {
    Serial.println("Could not find fingerprint features");
    return false;
  } else {
    Serial.println("Unknown error");
    return false;
  }

  Serial.println("Remove finger");
  p = 0;

  // Menunggu hingga jari diangkat
  while (p != FINGERPRINT_NOFINGER) {
    p = finger.getImage();
  }
  Serial.print("ID ");
  Serial.println(id);
  p = -1;

  Serial.print("ID ");
  Serial.println(id);
  p = finger.storeModel(id);
  if (p == FINGERPRINT_OK) {
    isEnroll = false;
    Serial.println("Stored!");

    doc["type"] = "fingerprint-set";
    doc["status"] = true;

    String jsonString = "";
    serializeJson(doc, jsonString);

    client.publish(topicMobile.c_str(), jsonString.c_str());

    delay(2000);

    isFingerActive = true;

    return true;
  } else {
    // Menangani kesalahan yang mungkin terjadi saat menyimpan model
    if (p == FINGERPRINT_PACKETRECIEVEERR) {
      Serial.println("Communication error");
    } else if (p == FINGERPRINT_BADLOCATION) {
      Serial.println("Could not store in that location");
    } else if (p == FINGERPRINT_FLASHERR) {
      Serial.println("Error writing to flash");
    } else {
      Serial.println("Unknown error");
    }
    return false;
  }
}


void getVibrate() {
  int sensorValue = digitalRead(getar);  // Baca status sensor getar.
  delay(250);

  Serial.print("Sensor Value: ");
  Serial.println(sensorValue);

  if (sensorValue > 0) {
    digitalWrite(buzzer, HIGH);        // Nyalakan buzzer.
    digitalWrite(ledPin_hijau, LOW);   // Matikan LED hijau.
    digitalWrite(ledPin_merah, HIGH);  // Nyalakan LED merah.
    digitalWrite(relay_dl, HIGH);      // Tutup gembok

    SendDeviceStatus();

    Serial.println("Ada Penyusup!");

    if (isNotificationActive) {
      sendNotification();
    }
  }
}

String receivedString = "";

void readSerialData() {
  String receivedData = "";  // String untuk menyimpan data yang diterima
  while (Serial.available() > 0) {
    char incomingChar = Serial.read();
    if (incomingChar == '\n') {
      // Proses data yang diterima setelah menerima newline ('\n')
      receivedData.trim();  // Hapus spasi ekstra dari awal dan akhir string

      Serial.print("Data numerik yang diterima: ");
      Serial.println(receivedData.toInt());  // Mengonversi string ke integer

      // Lakukan tindakan yang diperlukan dengan data numerik yang diterima
      int numericData = receivedData.toInt();
      if (numericData == 200) {
        Serial.println("Data numerik 200 diterima!");
        faceTrue = true;
      } else {
        Serial.println("Data numerik 400, tidak ada wajah cocok!");
        faceTrue = false;
      }

      receivedData = "";  // Reset String untuk menerima data selanjutnya
    } else {
      receivedData += incomingChar;  // Tambahkan karakter ke String
    }
  }
}

void loop() {

  // Serial.println(topicMobile);
  // Serial.print("FACETRUE: ");

  readSerialData();
  Serial.print("FACETRUE: ");
  Serial.println(faceTrue);

  Serial.print("STATUS GEMBOK: ");
  Serial.println(digitalRead(relay_dl) == HIGH);

  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  Serial.print("isEnroll:");
  Serial.println(isEnroll);

  if (isEnroll) {
    while (!getFingerprintEnroll());
  } else {
    if (isFingerActive) {
      getFingerprintID();
    } else {
      if (isCameraActive && faceTrue) {
        digitalWrite(relay_dl, LOW);       // Buka gembok
        digitalWrite(ledPin_hijau, HIGH);  // Nyalakan LED hijau.
        digitalWrite(buzzer, LOW);         //matikan buzzer
        digitalWrite(ledPin_merah, LOW);   // Matikan LED merah.

        delay(50);
        SendDeviceStatus();
      }
    }
  }

  delay(50);  // Memberi jeda.

  if (isVibrateActive) {
    getVibrate();
  }
}
