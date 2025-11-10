#include <Arduino.h>

#define RELEVADOR_PIN 9
#define PWR_PIN 10
#define RXD1_PIN 20
#define TXD1_PIN 21
#define BAUD_RATE 115200

HardwareSerial& gsmSerial = Serial1;

void enviarSMS(const String& numero, const String& mensaje) {
  Serial.print(">> Enviando SMS a ");
  Serial.println(numero);

  gsmSerial.print("AT+CMGS=\"");
  gsmSerial.print(numero);
  gsmSerial.println("\"");
  delay(1000);

  gsmSerial.print(mensaje);
  gsmSerial.write(26);  // Ctrl+Z
  delay(5000);

  Serial.println(">> SMS enviado.");
}

void imprimirRespuestaGsm() {
  String respuesta = "";
  while (gsmSerial.available()) {
    respuesta += (char)gsmSerial.read();
  }
  if (respuesta.length() > 0) {
    Serial.print("Respuesta del Módulo (Debug): ");
    Serial.println(respuesta);
  }
}

void borrarSMS(int indice) {
  Serial.print(">> Borrando mensaje en el índice: ");
  Serial.println(indice);
  gsmSerial.print("AT+CMGD=");
  gsmSerial.println(indice);
  delay(1000);
  imprimirRespuestaGsm();
}

String obtenerUbicacionGPS() {
  gsmSerial.println("AT+CGNSSPWR=1");  // Activar GNSS
  delay(1000);
  imprimirRespuestaGsm();

  for (int intento = 1; intento <= 10; intento++) {
    Serial.print(">> Intento ");
    Serial.println(intento);

    gsmSerial.println("AT+CGNSSINF");
    delay(2000);

    String respuesta = "";
    while (gsmSerial.available()) {
      respuesta += (char)gsmSerial.read();
    }

    respuesta.trim();

    if (respuesta.startsWith("+CGNSSINF:")) {
      // Ejemplo: +CGNSSINF: 0,1,20240630...,lat,lon,...
      int latStart = respuesta.indexOf(',', 20); // después del timestamp
      int latEnd = respuesta.indexOf(',', latStart + 1);
      int lonEnd = respuesta.indexOf(',', latEnd + 1);

      String lat = respuesta.substring(latStart + 1, latEnd);
      String lon = respuesta.substring(latEnd + 1, lonEnd);

      if (lat != "" && lon != "" && lat != "0.000000" && lon != "0.000000") {
        String link = "https://maps.google.com/?q=" + lat + "," + lon;
        Serial.println(">> Coordenadas GPS: " + link);
        return link;
      }
    }

    Serial.println(">> No se obtuvo ubicación. Reintentando...");
    delay(2000);
  }

  return "No se pudo obtener ubicación GPS.";
}

void revisarYProcesarSMS() {
  Serial.println("Buscando mensajes...");
  gsmSerial.println("AT+CMGL=\"REC UNREAD\"");
  delay(2000);

  String respuesta = "";
  while (gsmSerial.available()) {
    respuesta += (char)gsmSerial.read();
  }

  respuesta.trim();

  if (respuesta.length() > 0) {
    int inicioIndice = respuesta.indexOf("+CMGL: ");
    if (inicioIndice != -1) {
      int finLinea = respuesta.indexOf('\n', inicioIndice);
      String cabecera = respuesta.substring(inicioIndice, finLinea);

      int primerComa = cabecera.indexOf(',');
      String indiceStr = cabecera.substring(7, primerComa);
      int indiceMensaje = indiceStr.toInt();

      int com1 = cabecera.indexOf('"');
      int com2 = cabecera.indexOf('"', com1 + 1);
      int com3 = cabecera.indexOf('"', com2 + 1);
      int com4 = cabecera.indexOf('"', com3 + 1);
      String numeroRemitente = cabecera.substring(com3 + 1, com4);

      String cuerpoMensaje = respuesta.substring(finLinea + 1);
      cuerpoMensaje.trim();

      if (cuerpoMensaje.endsWith("OK")) {
        cuerpoMensaje = cuerpoMensaje.substring(0, cuerpoMensaje.length() - 2);
        cuerpoMensaje.trim();
      }

      Serial.print(">> Mensaje recibido: '");
      Serial.print(cuerpoMensaje);
      Serial.println("'");

      if (cuerpoMensaje.equalsIgnoreCase("Prender")) {
        digitalWrite(RELEVADOR_PIN, HIGH);
        enviarSMS(numeroRemitente, "Relevador encendido.");
      }
      else if (cuerpoMensaje.equalsIgnoreCase("Apagar")) {
        digitalWrite(RELEVADOR_PIN, LOW);
        enviarSMS(numeroRemitente, "Relevador apagado.");
      }
      else if (cuerpoMensaje.equalsIgnoreCase("Localizar")) {
        Serial.println(">> COMANDO: Obteniendo ubicación GPS...");
        String ubicacion = obtenerUbicacionGPS();
        enviarSMS(numeroRemitente, ubicacion);
      }
      else {
        enviarSMS(numeroRemitente, "Comando no reconocido.");
      }

      borrarSMS(indiceMensaje);
    }
  }
}

void setup() {
  Serial.begin(BAUD_RATE);
  delay(1000);

  pinMode(RELEVADOR_PIN, OUTPUT);
  digitalWrite(RELEVADOR_PIN, LOW);

  pinMode(PWR_PIN, OUTPUT);
  digitalWrite(PWR_PIN, LOW);
  delay(100);
  digitalWrite(PWR_PIN, HIGH);
  delay(2000);
  digitalWrite(PWR_PIN, LOW);

  gsmSerial.begin(BAUD_RATE, SERIAL_8N1, RXD1_PIN, TXD1_PIN);

  delay(10000);
  gsmSerial.println("AT+CMGF=1");  // Modo texto
  delay(1000);
  imprimirRespuestaGsm();

  Serial.println("Sistema listo para recibir SMS.");
}

void loop() {
  revisarYProcesarSMS();
  delay(5000);
}
