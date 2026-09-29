#include <Wire.h>
#include <SPI.h>
#include <SD.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_SSD1306.h>

#define Sda 6
#define Scl 7
#define Sdcs 21
#define Spi_Sck  19
#define Spi_Miso 20
#define Spi_Mosi 22


Adafruit_MPU6050 mpu;
Adafruit_SSD1306 display(128, 64, &Wire, -1);


unsigned long Time_Oled = 0;
unsigned long Time_Sd = 0;

void setup() {

  Serial.begin(115200);
  delay(500);

  // Inicializacao do barramento I2C
  Wire.begin(Sda, Scl);

  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println("falha para inicializar o oled");
  } else {
    Serial.println("oled sincronizado");
  }
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);

  // Inicializacao do MPU6050
  if (!mpu.begin()) {
    Serial.println("falha de leitura do I2C mpu");
  }

  SPI.begin(Spi_Sck, Spi_Miso, Spi_Mosi, Sdcs);
  if (!SD.begin(Sdcs)) {
    Serial.println("falha na");
  } else {
    Serial.println("sinais salvos no sd");
  }
}

void loop() {
  sensors_event_t a, g, temp;
  if (!mpu.getEvent(&a, &g, &temp)) {
    Serial.println("falha na leitura");
    delay(500);
    return;
  }

  unsigned long currentMillis = millis();


  if (currentMillis - Time_Oled >= 500) {
    Time_Oled = currentMillis;

    display.clearDisplay();
    display.setTextSize(1);
    display.setCursor(0, 0);
    display.println("valores");
    display.print("X: "); display.print(a.acceleration.x, 1); display.println(" m/s2");
    display.print("Y: "); display.print(a.acceleration.y, 1); display.println(" m/s2");
    display.print("Z: "); display.print(a.acceleration.z, 1); display.println(" m/s2");
    display.display();
  }

  if (currentMillis - Time_Sd >= 1000) {
    Time_Sd = currentMillis;


    File logFile = SD.open("/jornada.txt", FILE_APPEND);
    if (logFile) {
      logFile.print("Tempo: ");
      logFile.print(currentMillis / 1000);
      logFile.print("s | Acc X: "); logFile.print(a.acceleration.x, 2);
      logFile.print(" | Y: "); logFile.print(a.acceleration.y, 2);
      logFile.print(" | Z: "); logFile.println(a.acceleration.z, 2);
      logFile.close();
      
      Serial.println("sinais salvos no cartao sd");
    } else {
      Serial.println("falha ao escrever no sd");
    }

  }
}