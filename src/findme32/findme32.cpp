#include <Arduino.h>
#include "config.h"
#include "GSMModule.h"
#include "GPSModule.h"
#include "HTTPClient.h"
#include "GeoUtils.h"

// ============================
// VARIABLES GLOBALES
// ============================
HardwareSerial& gsmSerial = Serial1;

GSMModule gsm(gsmSerial, PWR_PIN, RXD1_PIN, TXD1_PIN, BAUD_RATE);
GPSModule gps(gsmSerial);
HTTPClient httpClient(gsm);

unsigned long ultimoCheckGPS = 0;
unsigned long ultimoEnvioServidor = 0;
unsigned long tiempoUltimaLectura = 0;

int fallosGPSConsecutivos = 0;
const int MAX_FALLOS_GPS = 3;

bool posicionActualValida = false;
double lat_actual_leida = 0.0;
double lon_actual_leida = 0.0;
double lat_ultimo_envio = 0.0;
double lon_ultimo_envio = 0.0;

// ============================
// HELPER DE ENVÍO
// ============================
void enviarYActualizar(double lat, double lon, double speed = -1.0);

// ============================
// HELPER DE ENVÍO
// ============================
void enviarYActualizar(double lat, double lon, double speed) {
  if (httpClient.enviarUbicacion(lat, lon, speed)) {
    Serial.println(">> Envío exitoso. Actualizando posición base.");
    lat_ultimo_envio = lat;
    lon_ultimo_envio = lon;
    ultimoEnvioServidor = millis();
  } else {
    Serial.println(">> Falla de envío. Se reintentará en el próximo ciclo.");
  }
}


// ============================
// SETUP
// ============================
void setup() {
  Serial.begin(BAUD_RATE);
  delay(2000);
  Serial.println("\n\n>> =============================");
  Serial.println(">> GPS Tracker - FindMe32 (Modular)");
  Serial.println(">> =============================");
  Serial.println(">> Device Token: " + String(DEVICE_TOKEN));
  Serial.println(">> Umbral Movimiento: " + String(UMBRAL_MOVIMIENTO_METROS) + " metros");
  Serial.println(">> Intervalo GPS: 30 segundos");
  Serial.println(">> Intervalo Heartbeat: 5 minutos");
  Serial.println(">> =============================\n");
  
  // Inicializar pines de control
  pinMode(PIN_ACTIVE, OUTPUT);
  pinMode(PIN_INACTIVE, OUTPUT);
  digitalWrite(PIN_ACTIVE, LOW);
  digitalWrite(PIN_INACTIVE, LOW);
  Serial.println(">> Pines de control inicializados (" + String(PIN_ACTIVE) + ", " + String(PIN_INACTIVE) + ")");
  
  gsm.begin();
  
  if (!gsm.esperarRegistroRed()) {
    Serial.println(">> ✗ ADVERTENCIA: No se pudo registrar en la red");
  }
  
  gsm.verificarCalidadSenal();
  
  if (!gsm.verificarYSincronizarReloj()) {
    Serial.println(">> ✗ ADVERTENCIA: Problemas con sincronización de reloj");
  }
  
  if (!gps.inicializar()) {
    Serial.println(">> ✗ ADVERTENCIA: Error al inicializar GPS");
  }
  
  if (!gsm.verificarConexionGPRS()) {
    Serial.println(">> ✗ ADVERTENCIA: No se pudo configurar GPRS");
  }
  
  Serial.println("\n>> Sistema listo. Comenzando ciclo de envío...\n");
  Serial.println("\n>> Esperando primer 'fix' de GPS (puede tardar)...");
  ultimoCheckGPS = millis();
  ultimoEnvioServidor = millis();
}

// ============================
// LOOP PRINCIPAL
// ============================
void loop() {
  unsigned long tiempoActual = millis();

  // --- 1. LÓGICA DE LECTURA DE GPS (Cada 30 segundos) ---
  if (tiempoActual - ultimoCheckGPS >= INTERVALO_LECTURA_GPS) {
    ultimoCheckGPS = tiempoActual;
    
    if (!gsm.verificarConexionGPRS()) {
      Serial.println(">> Error: Sin conexión GPRS. Reintentando en 30s...");
      return;
    }

    GpsData pos = gps.obtenerCoordenadas(3);
    
    if (pos.valida) {
      fallosGPSConsecutivos = 0; // Resetear contador de fallos
      unsigned long tiempoActualLectura = millis();
      lat_actual_leida = pos.lat;
      lon_actual_leida = pos.lon;

      if (!posicionActualValida) {
        // --- CASO A: Es el primer fix válido ---
        Serial.println(">> Primera ubicación GPS obtenida. Enviando...");
        posicionActualValida = true;
        tiempoUltimaLectura = tiempoActualLectura;
        enviarYActualizar(lat_actual_leida, lon_actual_leida);
      
      } else {
        // --- CASO B: Ya teníamos un fix, comparar si hay movimiento ---
        double distancia = calcularDistancia(lat_ultimo_envio, lon_ultimo_envio, lat_actual_leida, lon_actual_leida);

        if (distancia > UMBRAL_MOVIMIENTO_METROS) {
          // Calcular velocidad: distancia (m) / tiempo (s) = m/s -> * 3.6 = km/h
          unsigned long tiempoTranscurrido = (tiempoActualLectura - tiempoUltimaLectura) / 1000; // segundos
          double velocidadKmh = -1.0;
          
          if (tiempoTranscurrido > 0) {
            double velocidadMs = distancia / (double)tiempoTranscurrido;
            velocidadKmh = velocidadMs * 3.6; // Convertir m/s a km/h
          }
          
          Serial.println(">> MOVIMIENTO DETECTADO (" + String(distancia, 1) + "m). Enviando...");
          tiempoUltimaLectura = tiempoActualLectura;
          enviarYActualizar(lat_actual_leida, lon_actual_leida, velocidadKmh);
        } else {
          Serial.println(">> Estacionario (Variación: " + String(distancia, 1) + "m). Esperando heartbeat...");
        }
      }
    } else {
      Serial.println(">> No se obtuvo fix de GPS en este ciclo.");
      fallosGPSConsecutivos++;
      
      if (fallosGPSConsecutivos >= MAX_FALLOS_GPS) {
        Serial.println(">> ❗ " + String(MAX_FALLOS_GPS) + " fallos consecutivos de GPS. Reiniciando módulo GPS...");
        
        // Apagar GPS
        gsm.getSerial().println("AT+CGNSSPWR=0");
        delay(2000);
        
        // Reiniciar GPS
        if (gps.inicializar()) {
          Serial.println(">> ✓ Módulo GPS reiniciado correctamente");
          fallosGPSConsecutivos = 0;
        } else {
          Serial.println(">> ✗ Error al reiniciar módulo GPS");
        }
      }
    }
  } // Fin del chequeo de 30 segundos


  // --- 2. LÓGICA DE HEARTBEAT (Cada 5 minutos) ---
  if (tiempoActual - ultimoEnvioServidor >= INTERVALO_HEARTBEAT) {
    Serial.println(">> Han pasado 5 min (Heartbeat). Verificando si hay que enviar...");

    if (posicionActualValida) {
      double dist_desde_ultimo_envio = calcularDistancia(lat_ultimo_envio, lon_ultimo_envio, lat_actual_leida, lon_actual_leida);

      if (dist_desde_ultimo_envio > 0.1) { 
         Serial.println(">> Enviando última ubicación conocida (Heartbeat)...");
         enviarYActualizar(lat_actual_leida, lon_actual_leida);
      } else {
         Serial.println(">> Heartbeat: Ubicación no ha cambiado desde el último envío. Omitiendo.");
         ultimoEnvioServidor = tiempoActual; // Reiniciar timer
      }

    } else {
      Serial.println(">> Heartbeat: Aún sin fix GPS válido. No se envía nada.");
      ultimoEnvioServidor = tiempoActual; // Reiniciar timer
    }
  } // Fin del chequeo de 5 minutos

  delay(1000); // Pequeño delay
}