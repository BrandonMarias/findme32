#include <Arduino.h>
#include "findme32httptest.h"

// Crear instancia del módulo de prueba HTTP
FindMe32HttpTest httpTest;

void setup() {
  // Inicializar Serial para debug
  Serial.begin(115200);
  delay(2000);
  
  Serial.println("\n\n========================================");
  Serial.println("   FindMe32 - Prueba HTTP/HTTPS");
  Serial.println("========================================");
  Serial.println("URL: https://tunel.ponlecoco.com/api/gps/gpstracker");
  Serial.println("Parámetros: lat=19.432608&lon=-99.133209");
  Serial.println("========================================\n");
  
  // Inicializar módulo GSM (Serial1, pines, etc.)
  Serial.println(">> Inicializando módulo GSM...");
  httpTest.begin();
  delay(3000);
  
  Serial.println(">> Esperando estabilización de red...");
  delay(10000); // Dar tiempo al módulo para registrarse en la red
  
  // Ejecutar prueba HTTP
  Serial.println("\n>> Ejecutando prueba HTTP...\n");
  bool resultado = httpTest.runTest();
  
  Serial.println("\n========================================");
  if (resultado) {
    Serial.println("✓ PRUEBA HTTP EXITOSA");
    Serial.println("  El servidor respondió correctamente");
  } else {
    Serial.println("✗ PRUEBA HTTP FALLIDA");
    Serial.println("  Revisar logs arriba para detalles");
  }
  Serial.println("========================================\n");
  
  Serial.println(">> Prueba completada. Reinicia para ejecutar de nuevo.");
}

void loop() {
  // No hacer nada en el loop, la prueba se ejecuta una vez en setup()
  delay(1000);
}
