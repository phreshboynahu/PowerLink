// --- CONFIGURACIÓN SENSOR ---
// Se define la sensibilidad del sensor ACS712 (cuántos voltios cambia por cada amperio medido).
// En este caso, 0.02 V/A significa que por cada 1A de corriente, el sensor varía 0.02V.
// El offset se usa como margen para filtrar ruido del sensor.
float Sensibilidad = 0.02;  
float offset = 0.100;      

// --- CONFIGURACIÓN RELÉ ---
// Pin digital donde está conectado el módulo relé que controla el paso de corriente.
const int relePin = 8;     

// --- NUEVAS CONSTANTES PARA UMBRALES ---
// Se definen límites que determinan cuándo hay cambios en la carga o picos peligrosos.
// - UMBRAL_CAMBIO_CARGA: si la corriente varía más del 100%, se considera que se enchufó algo nuevo.
// - UMBRAL_FALLO_PICO: si la variación supera el 30%, se considera un pico peligroso.
// - CONTEO_GRACIA: cantidad de segundos de espera para que el sistema se estabilice tras un cambio.
const float UMBRAL_CAMBIO_CARGA = 1.0;  
const float UMBRAL_FALLO_PICO = 0.3;    
const int CONTEO_GRACIA = 5;            

// --- VARIABLES GLOBALES ---
// corrienteMedia: guarda el promedio actual de corriente.
// lecturasDeGracia: contador del tiempo de estabilización tras detectar un cambio.
// primeraMedicion: indica si es la primera lectura (para inicializar valores).
// falloDetectado: indica si se detectó un pico y el relé fue apagado.
// controlManual: indica si el usuario activó el control manual por comandos.
float corrienteMedia = 0;
int lecturasDeGracia = 0;  
bool primeraMedicion = true;
bool falloDetectado = false;   
bool controlManual = false;    

void setup() {
  // Inicia la comunicación serial para enviar datos al monitor.
  Serial.begin(9600);
  // Configura el pin del relé como salida.
  pinMode(relePin, OUTPUT);
  // Inicialmente en LOW (relé encendido si es activo bajo).
  digitalWrite(relePin, LOW); 
  // Mensajes informativos para el usuario.
  Serial.println("✅ Sistema iniciado...");
  Serial.println("Comandos disponibles:");
  Serial.println(" - 'RESET' para reactivar relé tras un fallo");
  Serial.println(" - '1' para encender relé manualmente");
  Serial.println(" - '0' para apagar relé manualmente");
  Serial.println(" - 'AUTO' para volver a control automático");
}

void loop() {
  // --- LECTURA DE COMANDOS SERIAL ---
  // Permite controlar el relé mediante el monitor serial.
  if (Serial.available()) {
    String comando = Serial.readStringUntil('\n');
    comando.trim(); // Elimina espacios o saltos de línea.

    // Comando RESET: reanuda el funcionamiento tras un fallo.
    if (comando.equalsIgnoreCase("RESET")) {
      falloDetectado = false;
      digitalWrite(relePin, LOW);
      Serial.println("✅ Relé reactivado manualmente.");
      primeraMedicion = true;  
    } 
    // Comando 1: activa el relé manualmente.
    else if (comando == "1") {
      controlManual = true;
      digitalWrite(relePin, LOW);
      Serial.println("🔸 Relé encendido manualmente.");
    } 
    // Comando 0: apaga el relé manualmente.
    else if (comando == "0") {
      controlManual = true;
      digitalWrite(relePin, HIGH);
      Serial.println("🔸 Relé apagado manualmente.");
    } 
    // Comando AUTO: vuelve al control automático.
    else if (comando.equalsIgnoreCase("AUTO")) {
      controlManual = false;
      Serial.println("🔄 Control automático activado.");
    }
  }

  // --- MODO MANUAL ---
  // Si el usuario está controlando el relé, se omiten las mediciones.
  if (controlManual) {
    delay(500);
    return;
  }

  // --- MODO AUTOMÁTICO ---
  // Si hay un fallo activo, no se hacen nuevas mediciones.
  if (falloDetectado) {
    delay(500);
    return;
  }

  // Lectura actual de corriente (valor pico).
  float Ip = get_corriente();     
  // Conversión a valor eficaz (RMS) para obtener corriente real.
  float Irms = Ip * 0.707;          
  // Cálculo estimado de potencia con 220V.
  float P = Irms * 220.0;         

  float variacion = 0;
  // Si ya hay una corriente media establecida, calcula cuánto varió respecto a ella.
  if (corrienteMedia > 0.02) { 
       variacion = abs(Irms - corrienteMedia) / corrienteMedia;
  } 
  // Si la media era casi cero, pero ahora hay corriente, considera un cambio total.
  else if (Irms > 0.02) {
       variacion = UMBRAL_CAMBIO_CARGA + 0.1; 
  }

  // Muestra los valores leídos por el monitor serial.
  Serial.print("Irms: ");
  Serial.print(Irms, 3);
  Serial.print(" A, Media: ");
  Serial.print(corrienteMedia, 3);
  Serial.print(" A, Variacion: ");
  Serial.print(variacion * 100, 2);
  Serial.print("% -> ");

  // --- LÓGICA DE DECISIÓN ---

  // CASO 1: Se detectó un cambio de carga (nuevo dispositivo enchufado).
  // Inicia un período de gracia para evitar falsas detecciones de picos.
  if (variacion > UMBRAL_CAMBIO_CARGA && lecturasDeGracia == 0) {
    Serial.print("Detectado cambio de carga. Estabilizando por ");
    Serial.print(CONTEO_GRACIA);
    Serial.println(" seg...");
    primeraMedicion = true;   
    lecturasDeGracia = CONTEO_GRACIA; 
    digitalWrite(relePin, LOW); 

  // CASO 2: Se detectó un pico de corriente peligroso.
  // Apaga el relé para proteger el circuito.
  } else if (variacion > UMBRAL_FALLO_PICO && lecturasDeGracia == 0) { 
    digitalWrite(relePin, HIGH);
    falloDetectado = true;
    Serial.println("⚠️ Pico de corriente detectado. Relé OFF. Esperando RESET manual.");

  // CASO 3: Todo normal o aún estabilizándose.
  } else {
    digitalWrite(relePin, LOW);

    if (lecturasDeGracia > 0) {
      // Aún dentro del período de gracia: se ajusta rápido a la nueva carga.
      Serial.println("OK. (Estabilizando...)");
      corrienteMedia = Irms; 
      lecturasDeGracia--; 
      if (lecturasDeGracia == 0) {
        Serial.println("...Estabilización completada. Monitoreo de picos reactivado.");
      }
    } else {
      // Estado normal: se actualiza la media lentamente.
      Serial.println("OK. Relé ON");
      corrienteMedia = 0.9 * corrienteMedia + 0.1 * Irms;
    }
  }

  // Si es la primera lectura, inicializa la media con el valor actual.
  if (primeraMedicion) {
    corrienteMedia = Irms; 
    primeraMedicion = false;
  }

  // Espera 1 segundo antes de repetir el ciclo.
  delay(1000);
}

// --- FUNCIÓN PARA MEDIR CORRIENTE ---
// Esta función calcula la corriente pico (Ip) midiendo los valores máximos y mínimos
// de voltaje en la salida del sensor ACS712 durante medio segundo.
float get_corriente() {
  float voltajeSensor;
  float corriente_instantanea;
  long tiempo = millis();
  
  // Inicializa valores extremos.
  float Imax = 0; 
  float Imin = 4096; 

  // Primer muestreo inicial.
  voltajeSensor = analogRead(A0);
  Imax = voltajeSensor;
  Imin = voltajeSensor;

  // Toma lecturas por 500 ms buscando los valores pico y valle.
  while (millis() - tiempo < 500) {
    voltajeSensor = analogRead(A0);
    if (voltajeSensor > Imax) {
      Imax = voltajeSensor;
    }
    if (voltajeSensor < Imin) {
      Imin = voltajeSensor;
    }
  }

  // Convierte los valores ADC (0–1023) a voltaje real (0–5V).
  float Vmax_real = Imax * (5.0 / 1023.0);
  float Vmin_real = Imin * (5.0 / 1023.0);

  // Calcula la amplitud del voltaje (diferencia pico a pico dividida entre 2).
  float Vpico = (Vmax_real - Vmin_real) / 2.0;

  // Convierte la amplitud de voltaje a corriente usando la sensibilidad del sensor.
  float Ip = Vpico / Sensibilidad;
  
  // Devuelve la corriente pico sin aplicar offset.
  return Ip;
}
