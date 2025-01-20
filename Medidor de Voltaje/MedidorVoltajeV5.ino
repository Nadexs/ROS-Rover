#include <Wire.h>                // Librería para la comunicación I2C (OLED)
#include <Adafruit_GFX.h>        // Librería para gráficos en OLED
#include <Adafruit_SSD1306.h>    // Librería para el controlador OLED SSD1306
#include <SoftwareSerial.h>      // Librería para la comunicación serial en pines digitales

#define SCREEN_WIDTH 128         // Ancho del OLED
#define SCREEN_HEIGHT 64         // Altura del OLED
#define OLED_RESET -1            // Reset (se puede dejar sin conectar)
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// Configuración de los pines para la comunicación serial por software
const int rxPin = 2;  // Pin de recepción (RX)
const int txPin = 3;  // Pin de transmisión (TX)
SoftwareSerial mySerial(rxPin, txPin);  // Crear el objeto para la comunicación serial

// Pines de los LEDs y buzzer
const int redLed = 11;
const int yellowLed = 10;
const int greenLed = 9;
const int buzzer = 8;

// Pines de estorbo >:v
const int pin1 = 3;
const int pin2 = 4;

// Pin del sensor de voltaje (conectado a A0)
const int voltagePin = A0;
float voltage = 0.0;
float voltage2 = 0.0;
const float voltageReference = 26.4;  // Referencia del voltaje actualizado
const int resolution = 1023;         // Resolución del ADC (10 bits)
const float voltageDividerFactor = 2; // Factor del divisor de voltaje

// Variables de control para los LEDs y buzzer
bool greenLedBlinking = false;
bool yellowLedBlinking = false;
bool redLedBlinking = false;
bool greenLedBlinkState = false;
bool yellowLedBlinkState = false;
bool redLedBlinkState = false;
unsigned long lastGreenBlinkTime = 0;
unsigned long lastYellowBlinkTime = 0;
unsigned long lastRedBlinkTime = 0;
int command = 0;
bool ledBlinking = false;
bool ledBlinkState = false;
unsigned long lastBlinkTime = 0;
const unsigned long blinkInterval = 500;  // Intervalo de parpadeo en milisegundos

// Variables para comunicación con la RPI 5
bool is_command = false; // ¿Se envía comando para la torreta?
bool is_ip = false;      // ¿Se envía la dirección IP?
bool is_lan = false;     // ¿Se envía el nombre de la red LAN?

String ipAddress = "";   // Variable para almacenar la dirección IP
String lanName = "";     // Variable para almacenar el nombre de la red LAN

void setup() {
  // Inicializar la comunicación serial por hardware para monitoreo
  Serial.begin(9600);

  // Inicializar pines de estorbo >:V
  pinMode(pin1, INPUT);
  pinMode(pin2, INPUT);

  // Inicializar la pantalla OLED
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {  // Dirección I2C de la pantalla OLED
    Serial.println(F("No se pudo encontrar el OLED"));
    for (;;);  // Ciclo infinito si no se encuentra la pantalla
  }

  // Limpiar buffer del OLED
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.print("Inicializando...");
  display.display();

  // Inicializar pines de LEDs y buzzer
  pinMode(redLed, OUTPUT);
  pinMode(yellowLed, OUTPUT);
  pinMode(greenLed, OUTPUT);
  pinMode(buzzer, OUTPUT);

  // Apagar todos los LEDs y buzzer al inicio
  digitalWrite(redLed, LOW);
  digitalWrite(yellowLed, LOW);
  digitalWrite(greenLed, LOW);
  digitalWrite(buzzer, LOW);
}

void loop() {
  // Leer el valor del sensor de voltaje (valor analógico entre 0 y 1023)
  int sensorValue = analogRead(voltagePin);

  // Convertir el valor a voltaje real
  voltage = (sensorValue * voltageReference) / resolution;
  voltage2 = (voltage * voltageDividerFactor);

  // Mostrar el voltaje en el OLED
  display.clearDisplay();
  display.setCursor(10, 0);
  display.setTextSize(3);
  display.setTextColor(SSD1306_WHITE);
  display.print(voltage2);
  display.println("V");

  // Mostrar el porcentaje estimado de batería
  int batteryPercentage = map(voltage2, 21.0, 29.4, 0, 100);
  batteryPercentage = constrain(batteryPercentage, 0, 100);
  display.setCursor(0, 30);
  display.setTextSize(1);
  display.print("Bateria: ");
  display.print(batteryPercentage);
  display.println("%");

  display.setCursor(0, 40);
  display.print("IP: ");
  display.print(ipAddress);

  display.setCursor(0, 50);
  display.print("LAN: ");
  display.print(lanName);
  display.display();

  // Enviar el voltaje por la comunicación serial por hardware
  Serial.println(voltage2);

  // Verificar si hay un comando entrante
  if (Serial.available()) {
    char chr = Serial.read();
    if (chr == 'C') {
      is_command = true;
      is_ip = false;
      is_lan = false;
    } else if (chr == 'I') {
      is_ip = true;
      is_command = false;
      is_lan = false;
      ipAddress = "";  // Resetear la IP antes de recibir la nueva
    } else if (chr == 'L') {
      is_lan = true;
      is_command = false;
      is_ip = false;
      lanName = "";  // Resetear el nombre de la red LAN antes de recibir el nuevo
    }

    if (is_command) {
      command = Serial.parseInt();  // Leer el comando como entero
      handleCommand(command);
    } else if (is_ip) {
      if (chr != 'I') {
        ipAddress += chr;
      }
    } else if (is_lan) {
      if (chr != 'L') {
        lanName += chr;
      }
    }
  }

  // Controlar el parpadeo de cada LED de forma independiente
  if (greenLedBlinking && millis() - lastGreenBlinkTime >= blinkInterval) {
    greenLedBlinkState = !greenLedBlinkState;
    digitalWrite(greenLed, greenLedBlinkState ? HIGH : LOW);
    lastGreenBlinkTime = millis();
  }

  if (yellowLedBlinking && millis() - lastYellowBlinkTime >= blinkInterval) {
    yellowLedBlinkState = !yellowLedBlinkState;
    digitalWrite(yellowLed, yellowLedBlinkState ? HIGH : LOW);
    lastYellowBlinkTime = millis();
  }

  if (redLedBlinking && millis() - lastRedBlinkTime >= blinkInterval) {
    redLedBlinkState = !redLedBlinkState;
    digitalWrite(redLed, redLedBlinkState ? HIGH : LOW);
    lastRedBlinkTime = millis();
  }

  // Verificar si el voltaje excede los 30V para iniciar la secuencia de barra de carga
  if (voltage2 > 30.0) {
    fadeInOut(greenLed);
    fadeInOut(yellowLed);
    fadeInOut(redLed);
  }

  delay(500);  // Ajustar el tiempo de delay si es necesario
}

void handleCommand(int cmd) {
  switch (cmd) {
    case 1: // Operación Normal
      greenLedBlinking = false;
      yellowLedBlinking = false;
      redLedBlinking = false;
      
      digitalWrite(greenLed, HIGH);
      digitalWrite(yellowLed, LOW);
      digitalWrite(redLed, LOW);
      
      greenLedBlinking = false;
      yellowLedBlinking = false;
      redLedBlinking = false;

      tone(buzzer, 1000, 200);  // Tono corto
      break;
    case 2: //Espera
      // Solo el LED verde parpadea
      greenLedBlinking = true;
      yellowLedBlinking = false;
      redLedBlinking = false;
      break;
    case 3: // Cargando Bateria
      greenLedBlinking = false;
      yellowLedBlinking = false;
      redLedBlinking = false;
      
      digitalWrite(greenLed, LOW);
      digitalWrite(yellowLed, HIGH);
      digitalWrite(redLed, LOW);
      
      digitalWrite(buzzer, LOW);
      
      break;
    case 4: // Bateria Baja
      // Solo el LED amarillo parpadea
      greenLedBlinking = false;
      yellowLedBlinking = true;
      redLedBlinking = false;
      break;
    case 5: // Error de Sistema
      greenLedBlinking = false;
      yellowLedBlinking = false;
      redLedBlinking = false;
      
      digitalWrite(greenLed, LOW);
      digitalWrite(yellowLed, LOW);
      digitalWrite(redLed, HIGH);
      
      digitalWrite(buzzer, LOW);
      
      break;
    case 6: // Desconexión HMI externa
      greenLedBlinking = false;
      yellowLedBlinking = false;
      redLedBlinking = true;
      digitalWrite(buzzer, LOW);
      break;
    case 7: // Activar parpadeo
      greenLedBlinking = true;
      yellowLedBlinking = true;
      redLedBlinking = true;
      break;
    case 8: // Activar buzzer
      tone(buzzer, 1000, 1000);
      break;
    case 9: // Caso prueba de funcionamiento LEDS HMI
      testPattern();
      break;
  }
}

// Función para hacer la transición suave (fade in/out)
void fadeInOut(int ledPin) {
  for (int brightness = 0; brightness <= 255; brightness += 5) {
    analogWrite(ledPin, brightness);
    delay(20);
  }

  for (int brightness = 255; brightness >= 0; brightness -= 5) {
    analogWrite(ledPin, brightness);
    delay(20);
  }
}

void testPattern() {
  for (int i = 0; i < 3; i++) {
    greenLedBlinking = false;
    yellowLedBlinking = false;
    redLedBlinking = false;
    
    digitalWrite(redLed, HIGH);
    delay(500);
    digitalWrite(redLed, LOW);

    digitalWrite(yellowLed, HIGH);
    delay(500);
    digitalWrite(yellowLed, LOW);

    digitalWrite(greenLed, HIGH);
    delay(500);
    digitalWrite(greenLed, LOW);

    tone(buzzer, 1000, 500);
    delay(1000);
    noTone(buzzer);
  }
}
