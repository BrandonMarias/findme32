#ifndef GSMMODULE_H
#define GSMMODULE_H

#include <Arduino.h>

/**
 * Clase para manejo del módulo GSM/GPRS
 * Gestiona: inicialización, registro en red, GPRS, sincronización de reloj
 */
class GSMModule {
public:
  GSMModule(HardwareSerial& serial, int pwrPin, int rxPin, int txPin, unsigned long baudRate);
  
  // Inicialización
  void begin();
  bool esperarRegistroRed(int maxIntentos = 30);
  
  // GPRS
  bool verificarConexionGPRS();
  bool estaContextoPDPActivo();
  
  // Sincronización de hora
  bool verificarYSincronizarReloj();
  
  // Utilidades
  String esperarRespuesta(unsigned long timeout_ms = 5000);
  void verificarCalidadSenal();
  
  HardwareSerial& getSerial() { return gsm; }

private:
  HardwareSerial& gsm;
  int pwrPin;
  int rxPin;
  int txPin;
  unsigned long baudRate;
  
  void encenderModulo();
  bool verificarComunicacion();
  bool necesitaSincronizarReloj(const String& reloj);
};

#endif // GSMMODULE_H
