#ifndef HTTPCLIENT_H
#define HTTPCLIENT_H

#include <Arduino.h>
#include "GSMModule.h"

/**
 * Cliente HTTP/HTTPS para envío de datos GPS
 */
class HTTPClient {
public:
  HTTPClient(GSMModule& gsmModule);
  
  bool enviarUbicacion(double lat, double lon, double speed = -1.0);
  
private:
  GSMModule& gsm;
  
  bool inicializarHTTP();
  String esperarRespuesta(unsigned long timeout_ms = 5000);
  bool parsearRespuestaHTTP(const String& respuesta, bool& isActive);
};

#endif // HTTPCLIENT_H
