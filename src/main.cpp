// --- Definición de Pines ---
#include <Arduino.h>
#define RELEVADOR_PIN 9 // Pin donde conectaste el relevador
#define PWR_PIN 10      // Pin conectado al PWRKEY del módulo
#define RXD1_PIN 20     // Pin RX del ESP32 (conectado al TXD del A7670SA)
#define TXD1_PIN 21     // Pin TX del ESP32 (conectado al RXD del A7670SA)
#define BAUD_RATE 115200 // Velocidad de baudios para la comunicación serial

// --- Objeto para la comunicación serial con el módulo ---
HardwareSerial& gsmSerial = Serial1;

// Función de utilidad para imprimir cualquier respuesta del módulo
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
  imprimirRespuestaGsm(); // Limpiar el buffer y mostrar respuesta
  Serial.println("-------------------------------------------------");
}

void revisarYProcesarSMS() {
  Serial.println("Buscando mensajes...");
  
  // Comando para leer todos los mensajes no leídos
  gsmSerial.println("AT+CMGL=\"REC UNREAD\"");
  
  // Dar tiempo para que llegue la respuesta completa
  delay(2000);

  String respuesta = "";
  while (gsmSerial.available()) {
    respuesta += (char)gsmSerial.read();
  }

  respuesta.trim(); // Limpiar espacios en blanco

  if (respuesta.length() > 0) {
    Serial.println("Respuesta del módulo:");
    Serial.println(respuesta);

    // Busca la cabecera de un mensaje
    if (respuesta.startsWith("+CMGL:")) {
      // --- Extraer el índice y el contenido del mensaje ---
      
      // Encontrar la primera coma para sacar el índice del mensaje
      int indiceComa = respuesta.indexOf(',');
      int indiceMensaje = respuesta.substring(indiceComa - 1, indiceComa).toInt();
      
      // Encontrar el inicio del cuerpo del mensaje (después del salto de línea \n)
      int inicioCuerpo = respuesta.indexOf('\n');
      // Extraer solo el cuerpo del mensaje y limpiarlo
      String cuerpoMensaje = respuesta.substring(inicioCuerpo + 1);
      cuerpoMensaje.trim();
      // A veces el comando OK se pega al final, lo quitamos.
      if (cuerpoMensaje.endsWith("OK")) {
        cuerpoMensaje = cuerpoMensaje.substring(0, cuerpoMensaje.length() - 2);
        cuerpoMensaje.trim();
      }

      Serial.print(">> Mensaje recibido: '");
      Serial.print(cuerpoMensaje);
      Serial.println("'");

      // --- Tomar decisiones basadas en el contenido ---
      if (cuerpoMensaje.equalsIgnoreCase("Prender")) {
        Serial.println(">> COMANDO: Encendiendo relevador...");
        digitalWrite(RELEVADOR_PIN, HIGH);
      } 
      else if (cuerpoMensaje.equalsIgnoreCase("Apagar")) {
        Serial.println(">> COMANDO: Apagando relevador...");
        digitalWrite(RELEVADOR_PIN, LOW);
      }
      else {
        Serial.println(">> COMANDO: No reconocido.");
      }
      
      // --- Borrar el mensaje procesado ---
      borrarSMS(indiceMensaje);
    }
  }
}



void setup() {
  // Iniciar comunicación para depuración (Monitor Serie)
  Serial.begin(BAUD_RATE);
  delay(1000); // Esperar a que la comunicación se establezca
  // --- Configurar el Relevador ---
  pinMode(RELEVADOR_PIN, OUTPUT);
  digitalWrite(RELEVADOR_PIN, LOW); // Iniciar con el relevador apagado
  Serial.println("Pin del relevador configurado como SALIDA y en estado BAJO.");

  // --- Encender el Módulo A7670SA ---
  pinMode(PWR_PIN, OUTPUT);
  Serial.println("Encendiendo el módulo A7670SA...");
  digitalWrite(PWR_PIN, LOW);
  delay(100);
  digitalWrite(PWR_PIN, HIGH);
  delay(2000);
  digitalWrite(PWR_PIN, LOW);
  
  // Iniciar comunicación serial con el módulo GSM
  gsmSerial.begin(BAUD_RATE, SERIAL_8N1, RXD1_PIN, TXD1_PIN);

  Serial.println("Módulo encendido. Esperando 10 segundos para que se configure...");
  delay(10000); // Dar tiempo al módulo para que se registre en la red

  // --- Configurar Módulo para SMS ---
  Serial.println("Configurando módulo en modo texto para SMS...");
  gsmSerial.println("AT+CMGF=1"); // Poner el módulo en modo texto
  delay(1000); // Pequeña espera para que el comando se procese
  imprimirRespuestaGsm(); // Imprime la respuesta para depurar

  Serial.println("Setup completo. El sistema está listo para recibir SMS.");
  Serial.println("-------------------------------------------------");
}

void loop() {
  // Revisa si hay nuevos SMS cada 5 segundos.
  // Puedes cambiar este tiempo si lo necesitas.
  revisarYProcesarSMS();
  delay(5000); 
}

