#include <DHT.h>

// Definición del sensor DHT11
#define DHTPIN 7       // Pin digital conectado al DHT11
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);

// Definimos los pines de los 4 relés
const int pinRele1 = 2;
const int pinRele2 = 3;
const int pinRele3 = 4;
const int pinRele4 = 5;

// Estados de los relés
bool estadoRele[4] = {false, false, false, false};

// Umbral de seguridad de temperatura (°C)
const float tempMax = 40.0;

void setup() {
  // Inicializamos los pines de salida
  pinMode(pinRele1, OUTPUT);
  pinMode(pinRele2, OUTPUT);
  pinMode(pinRele3, OUTPUT);
  pinMode(pinRele4, OUTPUT);

  // Inicializar apagados (HIGH = desactivado en módulos de relé comunes)
  digitalWrite(pinRele1, HIGH);
  digitalWrite(pinRele2, HIGH);
  digitalWrite(pinRele3, HIGH);
  digitalWrite(pinRele4, HIGH);

  // Inicializar comunicación Serial
  Serial.begin(9600);
  dht.begin();

  Serial.println("Sistema de control de 4 Relés + DHT11");
  Serial.println("Comandos por Serial:");
  Serial.println(" '11' -> Encender Relé 1");
  Serial.println(" '10' -> Apagar Relé 1");
  Serial.println(" '21' -> Encender Relé 2");
  Serial.println(" '20' -> Apagar Relé 2");
  Serial.println(" '31' -> Encender Relé 3");
  Serial.println(" '30' -> Apagar Relé 3");
  Serial.println(" '41' -> Encender Relé 4");
  Serial.println(" '40' -> Apagar Relé 4");
}

void loop() {
  // Lectura del sensor
  float temperatura = dht.readTemperature();

  // Si la lectura es válida
  if (!isnan(temperatura)) {
    Serial.print("Temperatura actual: ");
    Serial.println(temperatura);

    // Apagado de emergencia si supera el umbral
    if (temperatura > tempMax) {
      apagarTodosReles();
      Serial.println("⚠️ Temperatura alta detectada. Relés APAGADOS por seguridad.");
    }
  }

  // Procesamiento de comandos seriales
  if (Serial.available() >= 2) {
    char releID = Serial.read();   // número de relé (1–4)
    char comando = Serial.read();  // acción ('1' = encender, '0' = apagar)

    int releNum = releID - '1'; // convertir a índice [0..3]

    if (releNum >= 0 && releNum < 4) {
      if (comando == '1') {
        estadoRele[releNum] = true;
        activarRele(releNum);
        Serial.print("Relé ");
        Serial.print(releNum + 1);
        Serial.println(": ENCENDIDO");
      } else if (comando == '0') {
        estadoRele[releNum] = false;
        desactivarRele(releNum);
        Serial.print("Relé ");
        Serial.print(releNum + 1);
        Serial.println(": APAGADO");
      }
    }
  }
}

// Funciones auxiliares
void activarRele(int num) {
  int pin = obtenerPin(num);
  digitalWrite(pin, LOW);  // LOW activa
}

void desactivarRele(int num) {
  int pin = obtenerPin(num);
  digitalWrite(pin, HIGH); // HIGH desactiva
}

void apagarTodosReles() {
  for (int i = 0; i < 4; i++) {
    estadoRele[i] = false;
    desactivarRele(i);
  }
}

int obtenerPin(int num) {
  switch (num) {
    case 0: return pinRele1;
    case 1: return pinRele2;
    case 2: return pinRele3;
    case 3: return pinRele4;
  }
  return -1;
}
