#include <Arduino.h>

// --- Definición de Pines ---
#define RELEVADOR_PIN 9
#define PWR_PIN 10
#define RXD1_PIN 20
#define TXD1_PIN 21
#define BAUD_RATE 115200

HardwareSerial &gsmSerial = Serial1;

// --- Enviar un SMS al número indicado con el texto dado ---
void enviarSMS(const String &numero, const String &mensaje)
{
  Serial.print(">> Enviando SMS a ");
  Serial.println(numero);

  gsmSerial.print("AT+CMGS=\"");
  gsmSerial.print(numero);
  gsmSerial.println("\"");
  delay(1000); // Esperar el prompt '>'

  gsmSerial.print(mensaje);
  gsmSerial.write(26); // Ctrl+Z para enviar el SMS
  delay(5000);         // Tiempo para que el mensaje se envíe

  Serial.println(">> SMS enviado.");
}

// --- Mostrar respuesta del módulo (debug) ---
void imprimirRespuestaGsm()
{
  String respuesta = "";
  while (gsmSerial.available())
  {
    respuesta += (char)gsmSerial.read();
  }
  if (respuesta.length() > 0)
  {
    Serial.print("Respuesta del Módulo (Debug): ");
    Serial.println(respuesta);
  }
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

// --- Procesar SMS no leídos y ejecutar acciones ---
void revisarYProcesarSMS()
{
  Serial.println("Buscando mensajes...");

  gsmSerial.println("AT+CMGL=\"REC UNREAD\"");
  delay(2000);

  String respuesta = "";
  while (gsmSerial.available())
  {
    respuesta += (char)gsmSerial.read();
  }
  respuesta.trim();

  if (respuesta.length() > 0)
  {
    Serial.println("Respuesta del módulo:");
    Serial.println(respuesta);

    int inicioIndice = respuesta.indexOf("+CMGL: ");
    if (inicioIndice != -1)
    {
      int finLinea = respuesta.indexOf('\n', inicioIndice);
      String cabecera = respuesta.substring(inicioIndice, finLinea);
      Serial.print("Cabecera: ");
      Serial.println(cabecera);

      // Extraer índice del SMS
      int primerComa = cabecera.indexOf(',');
      String indiceStr = cabecera.substring(7, primerComa);
      int indiceMensaje = indiceStr.toInt();

      // Extraer número del remitente
      // Buscar el número entre el tercer y cuarto par de comillas
      int comilla1 = cabecera.indexOf('"');
      int comilla2 = cabecera.indexOf('"', comilla1 + 1);
      int comilla3 = cabecera.indexOf('"', comilla2 + 1);
      int comilla4 = cabecera.indexOf('"', comilla3 + 1);

      // Extraer número telefónico
      String numeroRemitente = cabecera.substring(comilla3 + 1, comilla4);
      Serial.print("Número del remitente: ");
      Serial.println(numeroRemitente);

      // Extraer el mensaje
      String cuerpoMensaje = respuesta.substring(finLinea + 1);
      cuerpoMensaje.trim();

      if (cuerpoMensaje.endsWith("OK"))
      {
        cuerpoMensaje = cuerpoMensaje.substring(0, cuerpoMensaje.length() - 2);
        cuerpoMensaje.trim();
      }

      Serial.print(">> Mensaje recibido: '");
      Serial.print(cuerpoMensaje);
      Serial.println("'");

      // --- Ejecutar acción según el mensaje ---
      if (cuerpoMensaje.equalsIgnoreCase("Prender"))
      {
        Serial.println(">> COMANDO: Encendiendo relevador...");
        digitalWrite(RELEVADOR_PIN, HIGH);
        enviarSMS(numeroRemitente, "Relevador encendido.");
      }
      else if (cuerpoMensaje.equalsIgnoreCase("Apagar"))
      {
        Serial.println(">> COMANDO: Apagando relevador...");
        digitalWrite(RELEVADOR_PIN, LOW);
        enviarSMS(numeroRemitente, "Relevador apagado.");
      }
      else
      {
        Serial.println(">> COMANDO: No reconocido.");
        enviarSMS(numeroRemitente, "Comando no reconocido.");
      }

      // Borrar el mensaje después de procesarlo
      borrarSMS(indiceMensaje);
    }
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

  gsmSerial.begin(BAUD_RATE, SERIAL_8N1, RXD1_PIN, TXD1_PIN);

  Serial.println("Esperando 10 segundos para registro de red...");
  delay(10000);

  Serial.println("Configurando módulo para SMS en modo texto...");
  gsmSerial.println("AT+CMGF=1");
  delay(1000);
  imprimirRespuestaGsm();

  Serial.println("Sistema listo para recibir y responder comandos SMS.");
  Serial.println("-------------------------------------------------");
}

// --- Bucle principal ---
void loop()
{
  revisarYProcesarSMS();
  delay(5000);
}
