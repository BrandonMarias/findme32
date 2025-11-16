#ifndef GPSMODULE_H
#define GPSMODULE_H

#include <Arduino.h>

/**
 * Estructura para datos GPS
 */
struct GpsData {
  double lat;
  double lon;
  bool valida;
};

/**
 * Clase para manejo del módulo GPS
 */
class GPSModule {
public:
  GPSModule(HardwareSerial& serial);
  
  bool inicializar();
  GpsData obtenerCoordenadas(int maxIntentos = 10);
  
private:
  HardwareSerial& gsm;
  
  String esperarRespuesta();
  bool parsearCGNSSINFO(const String& respuesta, GpsData& data);
};

#endif // GPSMODULE_H
