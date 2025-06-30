#include <Arduino.h>

// --- Definición de Pines ---
#define RELEVADOR_PIN 9
#define PWR_PIN 10
#define RXD1_PIN 20
#define TXD1_PIN 21
#define BAUD_RATE 115200

HardwareSerial &gsmSerial = Serial1; // Usamos Serial1 para comunicación con el módulo GSM/GPS

// --- Función para leer toda la respuesta del módulo GSM ---
String leerRespuestaGsm(unsigned long timeout) {
    String respuesta = "";
    unsigned long inicioTiempo = millis();
    while (millis() - inicioTiempo < timeout) {
        if (gsmSerial.available()) {
            respuesta += (char)gsmSerial.read();
        }
    }
    return respuesta;
}

// --- Mostrar respuesta del módulo (debug) ---
void imprimirRespuestaGsm() {
    String respuesta = leerRespuestaGsm(1000); // Lee por 1 segundo para debug
    if (respuesta.length() > 0) {
        Serial.print("Respuesta del Módulo (Debug): ");
        Serial.println(respuesta);
    }
}

// --- Enviar un SMS al número indicado con el texto dado ---
void enviarSMS(const String &numero, const String &mensaje) {
    Serial.print(">> Enviando SMS a ");
    Serial.println(numero);

    gsmSerial.print("AT+CMGS=\"");
    gsmSerial.print(numero);
    gsmSerial.println("\"");

    delay(1000); // Esperar '>' del módem

    gsmSerial.print(mensaje); // Solo el texto
    delay(300);              // Pequeña espera
    gsmSerial.write(26);     // Ctrl+Z
    delay(5000);             // Esperar envío

    imprimirRespuestaGsm(); // Verifica la respuesta
    Serial.println(">> SMS enviado.");
}

// --- Borrar SMS por índice ---
void borrarSMS(int indice) {
    Serial.print(">> Borrando mensaje en el índice: ");
    Serial.println(indice);
    gsmSerial.print("AT+CMGD=");
    gsmSerial.println(indice);
    delay(1000);
    imprimirRespuestaGsm();
    Serial.println("-------------------------------------------------");
}

// --- Obtener ubicación GPS con reintentos ---
String obtenerUbicacionGPS() {
    Serial.println(">> Activando GPS...");
    gsmSerial.println("AT+CGNSSPWR=1"); // Activar el módulo GNSS
    delay(1000); // Esperar un poco a que el comando se procese
    imprimirRespuestaGsm(); // Imprimir la respuesta de activación

    // Esperar a que el GPS se inicialice y obtenga una posición inicial
    Serial.println(">> Esperando señal GPS (hasta 10 segundos)...");
    delay(10000); // Espera inicial más larga para la primera fijación

    String ubicacionURL = "No se pudo obtener ubicacion GPS."; // Valor por defecto

    for (int intento = 1; intento <= 100; intento++) {
        Serial.print(">> Intento ");
        Serial.print(intento);
        Serial.println(" para obtener ubicación GPS...");

        gsmSerial.println("AT+CGNSSINFO"); // Solicitar información GNSS
        String respuestaCompleta = leerRespuestaGsm(5000); // Leer respuesta por 5 segundos

        Serial.println("Respuesta cruda del módulo:");
        Serial.println(respuestaCompleta);

        // Buscar la línea que contiene la información GPS
        int inicioInfo = respuestaCompleta.indexOf("+CGNSSINFO:");
        if (inicioInfo != -1) {
            int finInfo = respuestaCompleta.indexOf('\n', inicioInfo);
            String lineaGps = respuestaCompleta.substring(inicioInfo, finInfo);
            lineaGps.trim(); // Limpiar espacios en blanco y saltos de línea

            Serial.print("Línea GPS extraída: ");
            Serial.println(lineaGps);

            // Dividir la línea GPS por comas
            String datos[20]; // Aumentamos el tamaño para mayor seguridad
            int campo = 0;
            int lastIndex = 0;
            for (int i = 0; i < lineaGps.length(); i++) {
                if (lineaGps.charAt(i) == ',') {
                    datos[campo++] = lineaGps.substring(lastIndex, i);
                    lastIndex = i + 1;
                    if (campo >= 20) break; // Evitar desbordamiento
                }
            }
            datos[campo++] = lineaGps.substring(lastIndex); // Último campo

            // Verificar que tengamos suficientes campos para latitud y longitud
            if (campo > 8) { // Necesitamos al menos hasta datos[8]
                String lat = datos[5]; // Latitud
                String ns = datos[6];  // Indicador N/S
                String lon = datos[7]; // Longitud
                String ew = datos[8];  // Indicador E/W

                Serial.print("Latitud: "); Serial.print(lat); Serial.print(" "); Serial.println(ns);
                Serial.print("Longitud: "); Serial.print(lon); Serial.print(" "); Serial.println(ew);

                // Validar que latitud y longitud no estén vacías o sean "0.0"
                if (lat.length() > 5 && lon.length() > 5 && lat != "0.0000000" && lon != "0.0000000") {
                    // Formatear con signo negativo si corresponde
                    if (ns == "S")
                        lat = "-" + lat;
                    if (ew == "W")
                        lon = "-" + lon;

                    ubicacionURL = "https://maps.google.com/?q=" + lat + "," + lon;
                    Serial.println(">> Coordenadas válidas encontradas:");
                    Serial.println(ubicacionURL);
                    break; // Salir del bucle de reintentos
                }
            }
        }
        Serial.println(">> No se obtuvo ubicación válida. Reintentando...");
        delay(2000); // Esperar antes del siguiente intento
    }

    Serial.println(">> Desactivando GPS...");
    gsmSerial.println("AT+CGNSSPWR=0"); // Desactivar el módulo GNSS
    delay(1000); // Esperar un poco
    imprimirRespuestaGsm(); // Imprimir respuesta de desactivación

    return ubicacionURL;
}

// --- Procesar SMS no leídos y ejecutar acciones ---
void revisarYProcesarSMS() {
    Serial.println("Buscando mensajes...");

    gsmSerial.println("AT+CMGL=\"REC UNREAD\""); // Leer mensajes no leídos
    String respuesta = leerRespuestaGsm(5000); // Leer respuesta por 5 segundos
    respuesta.trim();

    if (respuesta.length() > 0) {
        Serial.println("Respuesta completa del módulo al buscar SMS:");
        Serial.println(respuesta);

        // Buscar la primera ocurrencia de un mensaje no leído
        int inicioMensaje = respuesta.indexOf("+CMGL: ");
        if (inicioMensaje != -1) {
            int finCabecera = respuesta.indexOf('\n', inicioMensaje);
            String cabecera = respuesta.substring(inicioMensaje, finCabecera);
            Serial.print("Cabecera del SMS: ");
            Serial.println(cabecera);

            // Extraer índice del mensaje
            int primerComa = cabecera.indexOf(',');
            String indiceStr = cabecera.substring(7, primerComa);
            int indiceMensaje = indiceStr.toInt();

            // Extraer número del remitente
            // La lógica actual ya es correcta para extraer el número entre las comillas correctas.
            int comilla1 = cabecera.indexOf('"', primerComa); // Primer comilla después del índice
            int comilla2 = cabecera.indexOf('"', comilla1 + 1); // Cierra "REC UNREAD"
            int comilla3 = cabecera.indexOf('"', comilla2 + 1); // Abre el número
            int comilla4 = cabecera.indexOf('"', comilla3 + 1); // Cierra el número
            String numeroRemitente = cabecera.substring(comilla3 + 1, comilla4);
            Serial.print("Número del remitente: ");
            Serial.println(numeroRemitente);

            // Extraer cuerpo del mensaje
            String cuerpoMensaje = respuesta.substring(finCabecera + 1);
            cuerpoMensaje.trim(); // Limpiar espacios en blanco al inicio/final

            // Eliminar cualquier "OK" o caracteres de nueva línea/retorno de carro al final del mensaje
            while (cuerpoMensaje.endsWith("OK") || cuerpoMensaje.endsWith("\r") || cuerpoMensaje.endsWith("\n")) {
                if (cuerpoMensaje.endsWith("OK")) {
                    cuerpoMensaje = cuerpoMensaje.substring(0, cuerpoMensaje.length() - 2);
                }
                if (cuerpoMensaje.endsWith("\r")) {
                    cuerpoMensaje = cuerpoMensaje.substring(0, cuerpoMensaje.length() - 1);
                }
                if (cuerpoMensaje.endsWith("\n")) {
                    cuerpoMensaje = cuerpoMensaje.substring(0, cuerpoMensaje.length() - 1);
                }
                cuerpoMensaje.trim(); // Volver a limpiar después de cada eliminación
            }

            Serial.print(">> Mensaje recibido: '");
            Serial.print(cuerpoMensaje);
            Serial.println("'");

            // Procesar comandos
            if (cuerpoMensaje.equalsIgnoreCase("Prender")) {
                Serial.println(">> COMANDO: Encendiendo relevador...");
                digitalWrite(RELEVADOR_PIN, HIGH);
                enviarSMS(numeroRemitente, "Relevador encendido.");
            } else if (cuerpoMensaje.equalsIgnoreCase("Apagar")) {
                Serial.println(">> COMANDO: Apagando relevador...");
                digitalWrite(RELEVADOR_PIN, LOW);
                enviarSMS(numeroRemitente, "Relevador apagado.");
            } else if (cuerpoMensaje.equalsIgnoreCase("Localizar")) {
                Serial.println(">> COMANDO: Obteniendo ubicación GPS...");
                String ubicacion = obtenerUbicacionGPS();
                enviarSMS(numeroRemitente, ubicacion);
            } else {
                Serial.println(">> COMANDO: No reconocido.");
                enviarSMS(numeroRemitente, "Comando no reconocido.");
            }

            borrarSMS(indiceMensaje); // Borrar el SMS después de procesarlo
        } else {
            Serial.println("No se encontraron mensajes no leídos válidos.");
        }
    } else {
        Serial.println("No hay respuesta del módulo al buscar SMS.");
    }
}

// --- Configuración inicial ---
void setup() {
    Serial.begin(BAUD_RATE); // Inicializa la comunicación serial con el monitor
    delay(1000);

    pinMode(RELEVADOR_PIN, OUTPUT);
    digitalWrite(RELEVADOR_PIN, LOW); // Asegura que el relevador esté apagado al inicio
    Serial.println("Relevador configurado y apagado por defecto.");

    pinMode(PWR_PIN, OUTPUT);
    Serial.println("Encendiendo el módulo A7670SA...");
    // Secuencia de encendido del módulo A7670SA (pulso en PWR_PIN)
    digitalWrite(PWR_PIN, LOW);
    delay(100);
    digitalWrite(PWR_PIN, HIGH);
    delay(2000); // Mantener HIGH por 2 segundos
    digitalWrite(PWR_PIN, LOW);
    Serial.println("Pulso de encendido enviado.");

    // Inicializa la comunicación serial con el módulo GSM/GPS
    gsmSerial.begin(BAUD_RATE, SERIAL_8N1, RXD1_PIN, TXD1_PIN);
    Serial.print("Comunicación con el módulo GSM/GPS iniciada en pines RXD1=");
    Serial.print(RXD1_PIN);
    Serial.print(", TXD1=");
    Serial.print(TXD1_PIN);
    Serial.print(" a ");
    Serial.print(BAUD_RATE);
    Serial.println(" baudios.");

    Serial.println("Esperando 10 segundos para registro de red...");
    delay(10000); // Dar tiempo al módulo para registrarse en la red

    Serial.println("Configurando módulo para SMS en modo texto...");
    gsmSerial.println("AT+CMGF=1"); // Configurar SMS en modo texto
    delay(1000);
    imprimirRespuestaGsm(); // Imprimir la respuesta de la configuración de SMS

    Serial.println("Sistema listo para recibir y responder comandos SMS.");
    Serial.println("-------------------------------------------------");
}

// --- Bucle principal ---
void loop() {
    revisarYProcesarSMS(); // Revisa y procesa los SMS entrantes
    delay(5000);           // Espera 5 segundos antes de la siguiente revisión
}
