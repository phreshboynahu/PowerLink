// --- CONFIGURACIÓN SENSOR ---
float Sensibilidad = 0.1;  // Sensibilidad (V/A) para ACS712 de 20A
float offset = 0.100;      // Amplitud del ruido (ajustar según tu sensor)

// --- CONFIGURACIÓN RELÉ ---
const int relePin = 8;     // Pin digital conectado al módulo relé

// --- VARIABLES GLOBALES ---
float corrienteMedia = 0;    
bool primeraMedicion = true;
bool falloDetectado = false;   // Estado de fallo
bool controlManual = false;    // TRUE = control manual activado

void setup() {
  Serial.begin(9600);
  pinMode(relePin, OUTPUT);
  digitalWrite(relePin, LOW); // Relé activado (ajusta si es activo LOW)
  Serial.println("✅ Sistema iniciado...");
  Serial.println("Comandos disponibles:");
  Serial.println(" - 'RESET' para reactivar relé tras un fallo");
  Serial.println(" - '1' para encender relé manualmente");
  Serial.println(" - '0' para apagar relé manualmente");
  Serial.println(" - 'AUTO' para volver a control automático");
}

void loop() {
  // --- LECTURA DE COMANDOS SERIAL ---
  if (Serial.available()) {
    String comando = Serial.readStringUntil('\n');
    comando.trim();

    if (comando.equalsIgnoreCase("RESET")) {
      falloDetectado = false;
      digitalWrite(relePin, LOW);
      Serial.println("✅ Relé reactivado manualmente.");
      primeraMedicion = true;  
    } 
    else if (comando == "1") {
      controlManual = true;
      digitalWrite(relePin, LOW);
      Serial.println("🔸 Relé encendido manualmente.");
    } 
    else if (comando == "0") {
      controlManual = true;
      digitalWrite(relePin, HIGH);
      Serial.println("🔸 Relé apagado manualmente.");
    } 
    else if (comando.equalsIgnoreCase("AUTO")) {
      controlManual = false;
      Serial.println("🔄 Control automático activado.");
    }
  }

  // --- MODO MANUAL: NO SE HACEN MEDICIONES ---
  if (controlManual) {
    delay(500);
    return;
  }

  // --- MODO AUTOMÁTICO ---
  if (falloDetectado) {
    delay(500);
    return;
  }

  float Ip = get_corriente();       
  float Irms = Ip * 0.707;          
  float P = Irms * 220.0;           

  if (primeraMedicion) {
    corrienteMedia = Irms;
    primeraMedicion = false;
  }

  float variacion = abs(Irms - corrienteMedia) / corrienteMedia;

  Serial.print("Irms: ");
  Serial.print(Irms, 3);
  Serial.print(" A, Media: ");
  Serial.print(corrienteMedia, 3);
  Serial.print(" A, Variacion: ");
  Serial.print(variacion * 100, 2);
  Serial.print("% -> ");

  if (variacion > 0.30) {
    digitalWrite(relePin, HIGH);
    falloDetectado = true;
    Serial.println("⚠️ Corriente fuera de rango. Relé OFF. Esperando RESET manual.");
  } else {
    digitalWrite(relePin, LOW);
    Serial.println("OK. Relé ON");
    corrienteMedia = 0.9 * corrienteMedia + 0.1 * Irms;
  }

  delay(1000);
}

// --- FUNCIÓN PARA MEDIR CORRIENTE ---
float get_corriente() {
  float voltajeSensor;
  float corriente = 0;
  long tiempo = millis();
  float Imax = 0;
  float Imin = 0;

  while (millis() - tiempo < 500) {
    voltajeSensor = analogRead(A0) * (5.0 / 1023.0);
    corriente = 0.9 * corriente + 0.1 * ((voltajeSensor - 2.527) / Sensibilidad);
    if (corriente > Imax) Imax = corriente;
    if (corriente < Imin) Imin = corriente;
  }

  return ((Imax - Imin) / 2.0) - offset;
}
