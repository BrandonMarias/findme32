#include <Arduino.h>
#include "findme_config.h"

// --- Definición de Pines ---
#define RELEVADOR_PIN 9
#define PWR_PIN 10
#define RXD1_PIN 20
#define TXD1_PIN 21
#define BAUD_RATE 115200

HardwareSerial &gsmSerial = Serial1; // Usamos Serial1 para comunicación con el módulo GSM/GPS

bool esNumeroAutorizado(const String &numero)
{
  for (int i = 0; i < totalNumeros; i++)
  {
    if (numerosAutorizados[i] == numero)
    {
      return true;
    }
  }
  return false;
}

// --- Función para leer toda la respuesta del módulo GSM ---
String leerRespuestaGsm(unsigned long timeout)
{
  String respuesta = "";
  unsigned long inicioTiempo = millis();
  while (millis() - inicioTiempo < timeout)
  {
    if (gsmSerial.available())
    {
      respuesta += (char)gsmSerial.read();
    }
  }
  return respuesta;
}

// --- Mostrar respuesta del módulo (debug) ---
void imprimirRespuestaGsm()
{
  String respuesta = leerRespuestaGsm(1000); // Lee por 1 segundo para debug
  if (respuesta.length() > 0)
  {
    Serial.print("Respuesta del Módulo (Debug): ");
    Serial.println(respuesta);
  }
}

// --- Verifica si el módulo está conectado a la red ---
bool estaEnRed()
{
  gsmSerial.println("AT+CEREG?");
  String r = leerRespuestaGsm(2000);
  if (r.indexOf("+CEREG:") != -1)
  {
    int coma = r.indexOf(',');
    char s = r.charAt(coma + 1);
    return (s == '1' || s == '5');
  }
  return false;
}

// --- Reinicia la función de red del módulo ---
void reconectar()
{
  Serial.println("❗ Módulo sin red. Reiniciando radio...");
  gsmSerial.println("AT+CFUN=0");
  delay(500);
  gsmSerial.println("AT+CFUN=1");
  delay(15000);
  imprimirRespuestaGsm();
}

// --- Enviar un SMS al número indicado con el texto dado ---
void enviarSMS(const String &numero, const String &mensaje)
{
  Serial.print(">> Enviando SMS a ");
  Serial.println(numero);

  gsmSerial.print("AT+CMGS=\"");
  gsmSerial.print(numero);
  gsmSerial.println("\"");

  delay(1000); // Esperar '>' del módem

  gsmSerial.print(mensaje); // Solo el texto
  delay(300);               // Pequeña espera
  gsmSerial.write(26);      // Ctrl+Z
  delay(5000);              // Esperar envío

  imprimirRespuestaGsm(); // Verifica la respuesta
  Serial.println(">> SMS enviado.");
}

// --- Borrar SMS por índice ---
void borrarSMS(int indice)
{
  Serial.print(">> Borrando mensaje en el índice: ");
  Serial.println(indice);
  gsmSerial.print("AT+CMGD=");
  gsmSerial.println(indice);
  delay(1000);
  imprimirRespuestaGsm();
  Serial.println("-------------------------------------------------");
}

// --- Obtener ubicación GPS con reintentos ---
String obtenerUbicacionGPS()
{
  Serial.println(">> Activando GPS...");
  gsmSerial.println("AT+CGNSSPWR=1");
  delay(1000);
  imprimirRespuestaGsm();

  Serial.println(">> Esperando señal GPS (hasta 10 segundos)...");
  delay(10000);

  String ubicacionURL = "No se pudo obtener ubicacion GPS.";

  for (int intento = 1; intento <= 100; intento++)
  {
    Serial.print(">> Intento ");
    Serial.print(intento);
    Serial.println(" para obtener ubicación GPS...");

    gsmSerial.println("AT+CGNSSINFO");
    String respuestaCompleta = leerRespuestaGsm(5000);

    Serial.println("Respuesta cruda del módulo:");
    Serial.println(respuestaCompleta);

    int inicioInfo = respuestaCompleta.indexOf("+CGNSSINFO:");
    if (inicioInfo != -1)
    {
      int finInfo = respuestaCompleta.indexOf('\n', inicioInfo);
      String lineaGps = respuestaCompleta.substring(inicioInfo, finInfo);
      lineaGps.trim();

      String datos[20];
      int campo = 0;
      int lastIndex = 0;
      for (int i = 0; i < lineaGps.length(); i++)
      {
        if (lineaGps.charAt(i) == ',')
        {
          datos[campo++] = lineaGps.substring(lastIndex, i);
          lastIndex = i + 1;
          if (campo >= 20)
            break;
        }
      }
      datos[campo++] = lineaGps.substring(lastIndex);

      if (campo > 8)
      {
        String lat = datos[5];
        String ns = datos[6];
        String lon = datos[7];
        String ew = datos[8];

        Serial.print("Latitud: ");
        Serial.print(lat);
        Serial.print(" ");
        Serial.println(ns);
        Serial.print("Longitud: ");
        Serial.print(lon);
        Serial.print(" ");
        Serial.println(ew);

        if (lat.length() > 5 && lon.length() > 5 && lat != "0.0000000" && lon != "0.0000000")
        {
          if (ns == "S")
            lat = "-" + lat;
          if (ew == "W")
            lon = "-" + lon;

          ubicacionURL = "https://maps.google.com/?q=" + lat + "," + lon;
          Serial.println(">> Coordenadas válidas encontradas:");
          Serial.println(ubicacionURL);
          break;
        }
      }
    }
    Serial.println(">> No se obtuvo ubicación válida. Reintentando...");
    delay(2000);
  }

  Serial.println(">> Desactivando GPS...");
  gsmSerial.println("AT+CGNSSPWR=0");
  delay(1000);
  imprimirRespuestaGsm();

  return ubicacionURL;
}

// --- Procesar SMS no leídos y ejecutar acciones ---
void revisarYProcesarSMS()
{
  Serial.println("Buscando mensajes...");

  gsmSerial.println("AT+CMGL=\"REC UNREAD\"");
  String respuesta = leerRespuestaGsm(5000);
  respuesta.trim();

  if (respuesta.length() > 0)
  {
    Serial.println("Respuesta completa del módulo al buscar SMS:");
    Serial.println(respuesta);

    int inicioMensaje = respuesta.indexOf("+CMGL: ");
    if (inicioMensaje != -1)
    {
      int finCabecera = respuesta.indexOf('\n', inicioMensaje);
      String cabecera = respuesta.substring(inicioMensaje, finCabecera);
      Serial.print("Cabecera del SMS: ");
      Serial.println(cabecera);

      int primerComa = cabecera.indexOf(',');
      String indiceStr = cabecera.substring(7, primerComa);
      int indiceMensaje = indiceStr.toInt();

      int comilla1 = cabecera.indexOf('"', primerComa);
      int comilla2 = cabecera.indexOf('"', comilla1 + 1);
      int comilla3 = cabecera.indexOf('"', comilla2 + 1);
      int comilla4 = cabecera.indexOf('"', comilla3 + 1);
      String numeroRemitente = cabecera.substring(comilla3 + 1, comilla4);
      Serial.print("Número del remitente: ");
      Serial.println(numeroRemitente);

      if (!esNumeroAutorizado(numeroRemitente))
      {
        Serial.println(">> Número NO autorizado. Ignorando mensaje.");
        enviarSMS(numeroRemitente, "No estás autorizado para usar este dispositivo.");
        borrarSMS(indiceMensaje); // Borra el mensaje aunque no sea válido
        return;                   // Sale de la función
      }

      String cuerpoMensaje = respuesta.substring(finCabecera + 1);
      cuerpoMensaje.trim();

      while (cuerpoMensaje.endsWith("OK") || cuerpoMensaje.endsWith("\r") || cuerpoMensaje.endsWith("\n"))
      {
        if (cuerpoMensaje.endsWith("OK"))
          cuerpoMensaje = cuerpoMensaje.substring(0, cuerpoMensaje.length() - 2);
        if (cuerpoMensaje.endsWith("\r"))
          cuerpoMensaje = cuerpoMensaje.substring(0, cuerpoMensaje.length() - 1);
        if (cuerpoMensaje.endsWith("\n"))
          cuerpoMensaje = cuerpoMensaje.substring(0, cuerpoMensaje.length() - 1);
        cuerpoMensaje.trim();
      }

      Serial.print(">> Mensaje recibido: '");
      Serial.print(cuerpoMensaje);
      Serial.println("'");

      if (cuerpoMensaje.equalsIgnoreCase("Apagar") || cuerpoMensaje.equalsIgnoreCase("Apagar "))
      {
        Serial.println(">> COMANDO: Encendiendo relevador...");
        digitalWrite(RELEVADOR_PIN, HIGH);
        enviarSMS(numeroRemitente, "Apagado");
      }
      else if (cuerpoMensaje.equalsIgnoreCase("Prender") || cuerpoMensaje.equalsIgnoreCase("Prender "))
      {
        Serial.println(">> COMANDO: Apagando relevador...");
        digitalWrite(RELEVADOR_PIN, LOW);
        enviarSMS(numeroRemitente, "Encendido");
      }
      else if (cuerpoMensaje.equalsIgnoreCase("Localizar"))
      {
        Serial.println(">> COMANDO: Obteniendo ubicación GPS...");
        String ubicacion = obtenerUbicacionGPS();
        enviarSMS(numeroRemitente, ubicacion);
      }
      else
      {
        Serial.println(">> COMANDO: No reconocido.");
        enviarSMS(numeroRemitente, "Comando no reconocido.");
      }

      borrarSMS(indiceMensaje);
    }
    else
    {
      Serial.println("No se encontraron mensajes no leídos válidos.");
    }
  }
  else
  {
    Serial.println("No hay respuesta del módulo al buscar SMS.");
  }
}

// --- Configuración inicial ---
void setup()
{
  Serial.begin(BAUD_RATE);
  delay(1000);

  pinMode(RELEVADOR_PIN, OUTPUT);
  digitalWrite(RELEVADOR_PIN, LOW);
  Serial.println("Relevador configurado y apagado por defecto.");

  pinMode(PWR_PIN, OUTPUT);
  Serial.println("Encendiendo el módulo A7670SA...");
  digitalWrite(PWR_PIN, LOW);
  delay(100);
  digitalWrite(PWR_PIN, HIGH);
  delay(2000);
  digitalWrite(PWR_PIN, LOW);
  Serial.println("Pulso de encendido enviado.");

  gsmSerial.begin(BAUD_RATE, SERIAL_8N1, RXD1_PIN, TXD1_PIN);
  Serial.print("Comunicación con el módulo GSM/GPS iniciada en pines RXD1=");
  Serial.print(RXD1_PIN);
  Serial.print(", TXD1=");
  Serial.print(TXD1_PIN);
  Serial.print(" a ");
  Serial.print(BAUD_RATE);
  Serial.println(" baudios.");

  Serial.println("Esperando 10 segundos para registro de red...");
  delay(10000);

  Serial.println("Configurando módulo para SMS en modo texto...");
  gsmSerial.println("AT+CMGF=1");
  delay(100);
  gsmSerial.println("AT+CREG=1");
  delay(100);
  gsmSerial.println("AT+CEREG=1");
  delay(100);
  gsmSerial.println("AT+CGREG=1");
  delay(100);
  imprimirRespuestaGsm();

  Serial.println("Sistema listo para recibir y responder comandos SMS.");
  Serial.println("-------------------------------------------------");
}

// --- Bucle principal ---
void loop()
{
  if (!estaEnRed())
    reconectar();
  revisarYProcesarSMS();
  delay(5000);
}