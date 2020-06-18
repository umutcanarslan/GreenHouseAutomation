#include <LiquidCrystal_I2C.h>
#include <dht11.h>
#include <Keypad.h>

//------Pinler------
int lightingR         = 2;
int lightingG         = 3;
int lightingB         = 4;
int buzzerPin         = 5;
int dht11Pin          = 7;
int waterPump         = 9;
int waterLevelR       = 41;
int waterLevelG       = 43;
int waterLevelB       = 45;
int heaterPin         = 51;
int gasSensor         = A0;
int soilMoisture      = A1;
int ldrSensor         = A3;
int waterLevelSensor  = A4;
int fanPin            = A5;

enum Warning {
  gas,          // Gaz uyarisi
};

enum Mode {
  tomato,       // A tusu
  banana,       // B tusu
  pepper,       // C tusu
  peach,        // D tusu
  defaultMode,  // 0 tusu
  notSelected,  // Secim yapilmamis mod. Mod secimi ve cikis durumunda
};

// Diger Degiskenler
const byte rowColumn  = 4;
char keys[rowColumn][rowColumn] = { {'1', '2', '3', 'A'},
                                    {'4', '5', '6', 'B'},
                                    {'7', '8', '9', 'C'},
                                    {'*', '0', '#', 'D'} };

byte rowPins[rowColumn] = {36, 34, 32, 30};   //4x4 Keypad için satir pinleri
byte colPins[rowColumn] = {28, 26, 24, 22};   //4x4 Keypad için sutun pinleri

// Sensorler ve Bilesenler //
Keypad keypad = Keypad(makeKeymap(keys), rowPins, colPins, rowColumn, rowColumn); // Keypad
LiquidCrystal_I2C lcd(0x27, 16, 2); // LCD
dht11 DHT11;

// Genel degiskenler //
bool isLogin = false;
Mode currentMode = notSelected;
String password = "0000";
String currentPassword = "";
bool isSoilRequire = true;
unsigned long currentSoilMilis = 0;

void setup() {
  Serial.begin(9600);
  Serial.println("Sistem Basladi...");
  setupPins();
  lcd.begin();
  lcd.backlight();
  welcomeMessages();
}

void loop() {
  char key = keypad.getKey();
      
  if (isLogin == false) {
    login(key);
  } else if (isLogin == true && currentMode == notSelected) {
    modeSelection(key);
  } else {
    displayCurrentMode(key);
  }
}

void setupPins() {
  pinMode(lightingR,        OUTPUT);
  pinMode(lightingG,        OUTPUT);
  pinMode(lightingB,        OUTPUT);
  pinMode(buzzerPin,        OUTPUT);
  pinMode(waterPump,        OUTPUT);
  pinMode(waterLevelR,      OUTPUT);
  pinMode(waterLevelG,      OUTPUT);
  pinMode(waterLevelB,      OUTPUT);
  pinMode(heaterPin,        OUTPUT);
  pinMode(fanPin,           OUTPUT);
  pinMode(gasSensor,        INPUT);
  pinMode(soilMoisture,     INPUT);
  pinMode(ldrSensor,        INPUT);
  pinMode(waterLevelSensor, INPUT);
}

void login(char key) {
  if (key == '#') {
    if (currentPassword == password) {
      isLogin = true;
      printLCD("= HOSGELDINIZ! =", " GIRIS YAPILDI. ");
      delay(1000);
      printLCD("MOD SECINIZ", " A, B, C, D, 0 ");
    } else {
      printLCD("SIFRE YANLISTIR.", "TEKRAR DENEYINIZ");
      delay(1000);
      printLCD("SIFRE GIRINIZ: ", "");
    }
    currentPassword = "";
  } else if (isDigit(key) == true) {
    currentPassword = currentPassword + key;
    String stars = "";
    for (int i = 0; i < currentPassword.length(); i++) { stars = stars + "*"; }
    printLCD("SIFRE GIRINIZ: ", stars);
  } else {
    return;
    // Herhangi bir islem yapilmaz. `Rakam` veya `#` degildir.
  }
}

void modeSelection(char key) {
  if (key == '#') {
    return;
  } else if (key == '*') {
    logout();
    welcomeMessages();
  } else if (isDigit(key)) {
    if (key == '0') {
      currentMode = defaultMode;
      printLCD(" OTOMATIK MOD : ", "----------------");
      delay(1000);    }
  } else {
    if (key == 'A') {
      currentMode = tomato;
      printLCD(" DOMATES MODU : ", "----------------");
      delay(1000);
    } else if (key == 'B') {  
      currentMode = banana;
      printLCD("   MUZ MODU:    ", "---------------");
      delay(1000);
    } else if (key == 'C') {
      currentMode = pepper;
      printLCD("   BIBER MODU:  ", "---------------");
      delay(1000);
    } else if (key == 'D') {
      currentMode = peach;
      printLCD("  SEFTALI MODU: ", "---------------");
      delay(1000);
    }
  }
}

void displayCurrentMode(char key) {
  if (currentMode == notSelected) {
    return;
  }
  if (key == '#') {
    printLCD("MOD SECINIZ", " A, B, C, D, 0 ");
    currentMode = notSelected;
    systemOff();
    return;
  }
  tempHumidityMonitoring();
  waterLevelMonitor();
  switch (currentMode) {
    case tomato:
      tomatoMode();
      break;
    case banana:
      bananaMode();
      break;
    case pepper:
      pepperMode();
      break;
    case peach:
      peachMode();
      break;
    case defaultMode:
      normalMode();
      break;
  }
  checkWaterPomping();
  gasWarnings();
}

void welcomeMessages() {
  printLCD("  HOSGELDINIZ!  ", "SERA OTOMASYONU!");
  delay(1500);
  printLCD("SIFRE GIRINIZ : ", "");
}

void logout() {
  isLogin = false;
  currentMode = notSelected;
  systemOff();
  printLCD(" CIKIS YAPILDI ", "----------------");
  delay(1500);
}

void systemOff() {
  // Sistem aydinlatmasinin kapatilmasi
  digitalWrite(lightingR, LOW);
  digitalWrite(lightingG, LOW);
  digitalWrite(lightingB, LOW);

  // Sogutucunun ve Isiticinin kapatilmasi
  digitalWrite(heaterPin, LOW);
  digitalWrite(fanPin, LOW);

  // Su seviyesi aydinlatmasinin kapatilmasi
  digitalWrite(waterLevelR, LOW);
  digitalWrite(waterLevelG, LOW);
  digitalWrite(waterLevelB, LOW);
  
  // Sulama sisteminin kapatilmasi
  digitalWrite(waterPump, LOW);
}

// ---- MODLAR ---- //

// Temp: 25, Soil: 940
void tomatoMode() {
  systemLighting(255, 0, 255);
  fanOrHeaterControl(25);
  soilControl(940);
}

// Temp: 26, Soil: 950
void bananaMode() {
  systemLighting(0, 30, 255);
  fanOrHeaterControl(26);
  soilControl(950);
}

// Temp: 23, Soil: 910
void pepperMode() {
  systemLighting(255, 255, 0);
  fanOrHeaterControl(23);
  soilControl(910);
}

// Temp: 24, Soil: 900
void peachMode() {
  systemLighting(128, 0, 128);
  fanOrHeaterControl(24);
  soilControl(900);
}

// Temp: 25, Soil: 930, no fanOrHeaterControl
void normalMode() {
  soilControl(930);
}

// ---- IZLEME VE KONTROL ETME ---- //
// Sistemin moda gore isiklandirmasi
void systemLighting (int R, int G, int B) {
  bool isNight = analogRead(ldrSensor) > 700; // 0..1024
  if (isNight == true) {
    digitalWrite(lightingR, R);
    digitalWrite(lightingG, G);
    digitalWrite(lightingB, B);
  } else {
    digitalWrite(lightingR, LOW);
    digitalWrite(lightingG, LOW);
    digitalWrite(lightingB, LOW);
  }
}

// Sicaklik ve Nem gozlemleme
void tempHumidityMonitoring() {
  DHT11.read(dht11Pin);
  String title    = "Sicaklik - Nem  ";
  String subtitle = String((float)DHT11.temperature, 1) + char(223) + "C";
  subtitle += "   - %" + String((float)DHT11.humidity, 1);
  printLCD(title, subtitle);
  delay(100);
}

// Su seviyesini olcer ve sartlar saglanirsa uyari gosterir.
void waterLevelMonitor() {
  int waterValue = analogRead(waterLevelSensor);

  if (waterValue < 300) {
    waterLighting(255, 0, 0);
    runWarnings(waterLevel);
  } else if (waterValue < 400) {
    waterLighting(255, 255, 0);
  } else {
    waterLighting(0, 255, 0);
  }
}

// Su seviyesi uyari isiklandirmasi
void waterLighting (int R, int G, int B) {
  digitalWrite(waterLevelR, R);
  digitalWrite(waterLevelG, G);
  digitalWrite(waterLevelB, B);
}

// Fan ve Isitici kontrolu
void fanOrHeaterControl(int modeRefTemp) {
  float temp = (float)DHT11.temperature;

  // +2, -2 min ve max degerlerdir.
  int min = modeRefTemp - 3;
  int max = modeRefTemp + 3;

  if (temp <= min) {
    digitalWrite(heaterPin, HIGH);
    digitalWrite(fanPin, LOW);
  } else if (temp >= max) {
    digitalWrite(heaterPin, LOW);
    digitalWrite(fanPin, HIGH);
  } else {
    digitalWrite(heaterPin, LOW);
    digitalWrite(fanPin, LOW);   
  }
}

// Sulama sistemi
void soilControl(int modeRefMoisture) {
  if (isSoilRequire == true) {
    // Sulama devam ediyor.
    return;
  }

  // 0 .. 1024 => 0...<ref, islak. Aksi durumda toprak: kuru
  int soilValue = analogRead(soilMoisture);

  if (soilValue > modeRefMoisture) {
    isSoilRequire = true;
    currentSoilMilis = millis();
    digitalWrite(waterPump, HIGH);
  } else {
    digitalWrite(waterPump, LOW);
  }
  Serial.println(soilValue);
}

void checkWaterPomping() {
  // millis, delay fonksiyonundan daha kullanisli. Sistemi durdurmadan,
  // sistem saati uzerinden sayac ile 2 sn sonra durduruyoruz.
  if (millis() > currentSoilMilis + 2000) {
    // Sulama durmali mi?
    digitalWrite(waterPump, LOW);
    isSoilRequire = false;
  } else {
    // Sulama devam ediyor.
  }
}

void printLCD(String row1, String row2) {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(row1);
  Serial.println(row1);
  lcd.setCursor(0, 1);
  lcd.print(row2);
  Serial.println(row2);
}

void gasWarnings(Warning warning) {
  int gazValue = analogRead(gasSensor);

  if (gazValue > 390) {
    digitalWrite(buzzerPin, HIGH);
    delay(500);
    digitalWrite(buzzerPin, LOW);
    delay(500);
  } else {
    digitalWrite(buzzerPin, LOW);
  }
}
