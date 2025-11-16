#include "HTTPClient.h"
#include "config.h"

HTTPClient::HTTPClient(GSMModule& gsmModule) : gsm(gsmModule) {}

String HTTPClient::esperarRespuesta(unsigned long timeout_ms) {
  String respuesta = "";
  unsigned long timeout = millis() + timeout_ms;
  HardwareSerial& gsmSerial = gsm.getSerial();
  
  while (millis() < timeout) {
    while (gsmSerial.available()) {
      respuesta += (char)gsmSerial.read();
    }
    if (respuesta.indexOf("OK") != -1 || respuesta.indexOf("ERROR") != -1) {
      break;
    }
    delay(100);
  }
  
  return respuesta;
}

bool HTTPClient::inicializarHTTP() {
  Serial.println(">> Inicializando HTTPS...");
  HardwareSerial& gsmSerial = gsm.getSerial();
  
  gsmSerial.println("AT+HTTPTERM");
  delay(500);
  
  gsmSerial.println("AT+HTTPINIT");
  delay(1000);
  String resp = esperarRespuesta();
  
  if (resp.indexOf("OK") == -1) {
    Serial.println(">> Error al inicializar HTTP");
    return false;
  }
  
  gsmSerial.println("AT+HTTPPARA=\"CID\",1");
  delay(500);
  esperarRespuesta();
  
  Serial.println(">> Habilitando SSL/TLS...");
  gsmSerial.println("AT+HTTPSSL=1");
  delay(500);
  esperarRespuesta();
  
  Serial.println(">> Configurando validación SSL...");
  gsmSerial.println("AT+CSSLCFG=\"sslversion\",0,3"); // TLS 1.2
  delay(500);
  esperarRespuesta();
  
  gsmSerial.println("AT+CSSLCFG=\"authmode\",0,0");
  delay(500);
  esperarRespuesta();

  Serial.println(">> Habilitando SNI (Server Name Indication)...");
  gsmSerial.println("AT+CSSLCFG=\"enableSNI\",0,1");
  delay(500);
  esperarRespuesta();
  
  return true;
}

bool HTTPClient::parsearRespuestaHTTP(const String& respuesta, bool& isActive) {
  int httpActionPos = respuesta.indexOf("+HTTPACTION:");
  if (httpActionPos == -1) {
    if (respuesta.indexOf("ERROR") != -1) {
      Serial.println(">> ✗ Error en comando AT");
    } else {
      Serial.println(">> ✗ Timeout o respuesta no reconocida");
    }
    return false;
  }
  
  String httpLine = respuesta.substring(httpActionPos);
  int lineEnd = httpLine.indexOf('\r');
  if (lineEnd != -1) {
    httpLine = httpLine.substring(0, lineEnd);
  }

  int firstComma = httpLine.indexOf(',');
  int secondComma = httpLine.indexOf(',', firstComma + 1);

  if (firstComma == -1 || secondComma == -1) {
    Serial.println(">> ✗ Error al parsear la respuesta +HTTPACTION");
    return false;
  }
  
  String statusCodeStr = httpLine.substring(firstComma + 1, secondComma);
  statusCodeStr.trim();
  int statusCode = statusCodeStr.toInt();

  String dataLenStr = httpLine.substring(secondComma + 1);
  dataLenStr.trim();
  int dataLen = dataLenStr.toInt();

  if (statusCode == 200 || statusCode == 201 || statusCode == 204) {
    Serial.println(">> ✓ Ubicación enviada exitosamente (" + statusCodeStr + ")");

    if (dataLen > 0) {
      Serial.println(">> Respuesta del servidor (" + String(dataLen) + " bytes):");
      HardwareSerial& gsmSerial = gsm.getSerial();
      delay(500);
      
      // Leer respuesta usando AT+HTTPREAD=<start>,<length>
      gsmSerial.print("AT+HTTPREAD=0,");
      gsmSerial.println(dataLen);
      delay(2000);
      
      String contenido = "";
      unsigned long readTimeout = millis() + 5000;
      while (millis() < readTimeout && gsmSerial.available()) {
        contenido += (char)gsmSerial.read();
      }
      Serial.println(contenido);
      
      // Extraer el valor de isActive del JSON
      int isActivePos = contenido.indexOf("\"isActive\":");
      if (isActivePos != -1) {
        int truePos = contenido.indexOf("true", isActivePos);
        int falsePos = contenido.indexOf("false", isActivePos);
        
        if (truePos != -1 && (falsePos == -1 || truePos < falsePos)) {
          isActive = true;
          Serial.println(">> Estado del dispositivo: ACTIVO");
        } else if (falsePos != -1) {
          isActive = false;
          Serial.println(">> Estado del dispositivo: INACTIVO");
        }
      }
    } else {
      Serial.println(">> Respuesta del servidor (sin contenido)");
    }
    
    return true;
  } else {
    Serial.println(">> ✗ Error HTTP " + statusCodeStr);
    if (statusCode == 715) {
      Serial.println(">> ERROR 715: Timeout SSL/TLS o certificado inválido");
    } else if (statusCode == 703) {
      Serial.println(">> ERROR 703: Error de DNS");
    } else if (statusCode == 714) {
      Serial.println(">> ERROR 714: Timeout HTTP");
    }
    return false;
  }
}

bool HTTPClient::enviarUbicacion(double lat, double lon, double speed) {
  HardwareSerial& gsmSerial = gsm.getSerial();
  
  Serial.println(">> Enviando ubicación al servidor...");
  
  // Verificar contexto PDP
  Serial.println(">> Verificando contexto PDP...");
  if (!gsm.estaContextoPDPActivo()) {
    Serial.println(">> Contexto PDP inactivo. Reactivando...");
    if (!gsm.verificarConexionGPRS()) {
      Serial.println(">> Error: No se pudo reactivar GPRS");
      return false;
    }
  }
  
  if (!inicializarHTTP()) {
    return false;
  }
  
  // Construir URL
  String url = "https://" + String(API_ENDPOINT) + String(API_PATH) + 
               "?lat=" + String(lat, 6) + 
               "&lon=" + String(lon, 6) +
               "&token=" + String(DEVICE_TOKEN);
  
  // Agregar velocidad si está disponible (speed >= 0)
  if (speed >= 0.0) {
    url += "&speed=" + String(speed, 1);
    Serial.println(">> Velocidad: " + String(speed, 1) + " km/h");
  }
  
  Serial.println(">> URL: " + url);
  
  gsmSerial.print("AT+HTTPPARA=\"URL\",\"");
  gsmSerial.print(url);
  gsmSerial.println("\"");
  delay(1000);
  String urlResp = esperarRespuesta();
  Serial.println(">> URL Config: " + urlResp);
  
  Serial.println(">> Ejecutando petición HTTP GET...");
  gsmSerial.println("AT+HTTPACTION=0");
  
  String respuesta = "";
  unsigned long timeout = millis() + HTTP_TIMEOUT;
  bool httpActionRecibido = false;
  
  while (millis() < timeout) {
    while (gsmSerial.available()) {
      char c = (char)gsmSerial.read();
      respuesta += c;
      Serial.print(c);
    }
    
    if (respuesta.indexOf("+HTTPACTION:") != -1) {
      httpActionRecibido = true;
      break;
    }
    
    if (respuesta.indexOf("+HTTP_NONET_EVENT") != -1) {
      Serial.println("\n>> ERROR: Sin conexión de red durante HTTP");
      break;
    }
    
    if (respuesta.indexOf("+CGEV: NW PDN DEACT") != -1) {
      Serial.println("\n>> ERROR: Contexto PDP desactivado durante HTTP");
      break;
    }
    
    delay(100);
  }
  
  Serial.println();
  
  if (!httpActionRecibido && (respuesta.indexOf("+HTTP_NONET_EVENT") != -1 || 
      respuesta.indexOf("+CGEV: NW PDN DEACT") != -1)) {
    Serial.println(">> Detectado error de red. Terminando HTTP y saliendo...");
    gsmSerial.println("AT+HTTPTERM");
    delay(500);
    esperarRespuesta();
    return false;
  }
  
  bool isActive = false;
  bool exito = parsearRespuestaHTTP(respuesta, isActive);
  
  // Controlar pines según el estado
  if (exito) {
    if (isActive) {
      digitalWrite(PIN_ACTIVE, HIGH);
      digitalWrite(PIN_INACTIVE, LOW);
      Serial.println(">> PIN " + String(PIN_ACTIVE) + " encendido, PIN " + String(PIN_INACTIVE) + " apagado");
    } else {
      digitalWrite(PIN_ACTIVE, LOW);
      digitalWrite(PIN_INACTIVE, HIGH);
      Serial.println(">> PIN " + String(PIN_ACTIVE) + " apagado, PIN " + String(PIN_INACTIVE) + " encendido");
    }
  }
  
  delay(500);
  gsmSerial.println("AT+HTTPTERM");
  delay(500);
  esperarRespuesta();
  
  return exito;
}
