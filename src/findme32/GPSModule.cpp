#include "GPSModule.h"
#include "config.h"

GPSModule::GPSModule(HardwareSerial& serial) : gsm(serial) {}

bool GPSModule::inicializar() {
  Serial.println(">> Inicializando GPS...");
  gsm.println("AT+CGNSSPWR=1");
  delay(1000);
  
  String respuesta = esperarRespuesta();
  Serial.println(">> Respuesta encendido GPS: " + respuesta);
  
  if (respuesta.indexOf("OK") != -1) {
    Serial.println(">> ✓ GPS encendido correctamente");
    return true;
  } else {
    Serial.println(">> Reintentando encendido GPS...");
    gsm.println("AT+CGNSSPWR=1");
    delay(1000);
    respuesta = esperarRespuesta();
    Serial.println(">> Respuesta reintento: " + respuesta);
    return (respuesta.indexOf("OK") != -1);
  }
}

String GPSModule::esperarRespuesta() {
  String respuesta = "";
  unsigned long timeout = millis() + 3000;
  
  while (millis() < timeout) {
    while (gsm.available()) {
      respuesta += (char)gsm.read();
    }
    if (respuesta.indexOf("OK") != -1 || respuesta.indexOf("ERROR") != -1) {
      break;
    }
    delay(50);
  }
  
  return respuesta;
}

bool GPSModule::parsearCGNSSINFO(const String& respuesta, GpsData& data) {
  if (respuesta.indexOf("+CGNSSINFO:") == -1) {
    return false;
  }
  
  int inicio = respuesta.indexOf("+CGNSSINFO:");
  int finLinea = respuesta.indexOf('\n', inicio);
  if (finLinea == -1) finLinea = respuesta.length();
  
  String linea = respuesta.substring(inicio + 12, finLinea);
  linea.trim();
  
  // Parsear campos separados por comas
  int campos[20];
  int campoCount = 0;
  campos[campoCount++] = -1;
  
  for (int i = 0; i < linea.length() && campoCount < 20; i++) {
    if (linea.charAt(i) == ',') {
      campos[campoCount++] = i;
    }
  }
  
  if (campoCount < 9) {
    return false;
  }
  
  // Extraer coordenadas
  String lat = linea.substring(campos[5] + 1, campos[6]);
  String latDir = linea.substring(campos[6] + 1, campos[7]);
  String lon = linea.substring(campos[7] + 1, campos[8]);
  String lonDir = linea.substring(campos[8] + 1, campos[9]);
  
  lat.trim();
  latDir.trim();
  lon.trim();
  lonDir.trim();
  
  if (lat.length() == 0 || lon.length() == 0) {
    return false;
  }
  
  float latNum = lat.toFloat();
  float lonNum = lon.toFloat();
  
  if (latNum == 0.0 || lonNum == 0.0) {
    return false;
  }
  
  // Aplicar dirección
  if (latDir == "S") latNum = -latNum;
  if (lonDir == "W") lonNum = -lonNum;
  
  data.lat = latNum;
  data.lon = lonNum;
  data.valida = true;
  
  return true;
}

GpsData GPSModule::obtenerCoordenadas(int maxIntentos) {
  Serial.println(">> Obteniendo coordenadas GPS...");
  GpsData data = {0.0, 0.0, false};

  for (int intento = 1; intento <= maxIntentos; intento++) {
    gsm.println("AT+CGNSSINFO");
    delay(2000);
    
    String respuesta = esperarRespuesta();
    Serial.println(">> Respuesta GPS: " + respuesta);
    
    if (parsearCGNSSINFO(respuesta, data)) {
      Serial.println(">> ✓ Coordenadas obtenidas: " + String(data.lat, 6) + "," + String(data.lon, 6));
      return data;
    }
    
    Serial.println(">> No se obtuvo ubicación válida. Reintentando...");
    delay(GPS_DELAY_INTENTO);
  }
  
  Serial.println(">> No se pudo obtener ubicación GPS en esta lectura.");
  return data;
}
