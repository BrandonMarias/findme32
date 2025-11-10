#include <Arduino.h>

// ============================
// CONFIGURACIÓN DEL DISPOSITIVO
// ============================
#define DEVICE_TOKEN "TU_TOKEN_UNICO_AQUI"  // Cambiar en cada dispositivo
#define API_ENDPOINT "mibackend.com"
#define API_PATH "/gpstracker"
#define API_PORT "80"  // Usar "443" para HTTPS

// ============================
// PINES Y CONFIGURACIÓN
// ============================
#define PWR_PIN 10
#define RXD1_PIN 20
#define TXD1_PIN 21
#define BAUD_RATE 115200

// ============================
// VARIABLES GLOBALES
// ============================
HardwareSerial& gsmSerial = Serial1;
unsigned long ultimoEnvio = 0;
const unsigned long INTERVALO_ENVIO = 5 * 60 * 1000;  // 5 minutos en milisegundos

// ============================
// FUNCIONES AUXILIARES
// ============================

void imprimirRespuesta() {
  String respuesta = "";
  unsigned long timeout = millis() + 5000;
  
  while (millis() < timeout) {
    while (gsmSerial.available()) {
      respuesta += (char)gsmSerial.read();
    }
    if (respuesta.indexOf("OK") != -1 || respuesta.indexOf("ERROR") != -1) {
      break;
    }
    delay(100);
  }
  
  if (respuesta.length() > 0) {
    Serial.println("Respuesta GSM: " + respuesta);
  }
}

String esperarRespuesta(unsigned long timeout_ms = 5000) {
  String respuesta = "";
  unsigned long timeout = millis() + timeout_ms;
  
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

// ============================
// FUNCIONES GPS
// ============================

bool inicializarGPS() {
  Serial.println(">> Inicializando GPS...");
  
  // Encender GPS/GNSS
  gsmSerial.println("AT+CGNSPWR=1");
  delay(1000);
  String resp = esperarRespuesta();
  
  if (resp.indexOf("OK") == -1 && resp.indexOf("ERROR") == -1) {
    Serial.println(">> Reintentando encendido GPS...");
    gsmSerial.println("AT+CGNSPWR=1");
    delay(1000);
    esperarRespuesta();
  }
  
  Serial.println(">> GPS inicializado");
  return true;
}

String obtenerCoordenadas() {
  Serial.println(">> Obteniendo coordenadas GPS...");
  
  for (int intento = 1; intento <= 5; intento++) {
    Serial.print(">> Intento ");
    Serial.print(intento);
    Serial.println(" de 5");
    
    gsmSerial.println("AT+CGNSINF");
    delay(2000);
    
    String respuesta = esperarRespuesta(3000);
    respuesta.trim();
    
    // Formato SIM7600: +CGNSINF: <GNSS run status>,<Fix status>,<UTC date & Time>,
    // <Latitude>,<Longitude>,<MSL Altitude>,<Speed Over Ground>,<Course Over Ground>,...
    // Ejemplo: +CGNSINF: 1,1,20231109120000.000,19.432608,-99.133209,525.4,0.0,0.0,1,,1.2,1.5,0.9,,15,7,,,45,,
    
    if (respuesta.indexOf("+CGNSINF:") != -1) {
      Serial.println(">> Respuesta GPS: " + respuesta);
      
      int inicio = respuesta.indexOf("+CGNSINF:");
      String datos = respuesta.substring(inicio + 10); // Saltar "+CGNSINF: "
      datos.trim();
      
      // Remover posibles caracteres extra al final
      int finLinea = datos.indexOf('\r');
      if (finLinea == -1) finLinea = datos.indexOf('\n');
      if (finLinea != -1) datos = datos.substring(0, finLinea);
      
      // Parsear los datos separados por comas
      int comas[25];
      int comaCount = 0;
      
      for (int i = 0; i < datos.length() && comaCount < 25; i++) {
        if (datos.charAt(i) == ',') {
          comas[comaCount++] = i;
        }
      }
      
      if (comaCount >= 4) {
        // Campo 0: GNSS run status (antes de la primera coma)
        String gnssRun = datos.substring(0, comas[0]);
        // Campo 1: Fix status (entre coma 0 y 1)
        String fixStatus = datos.substring(comas[0] + 1, comas[1]);
        // Campo 2: UTC time (entre coma 1 y 2)
        // Campo 3: Latitude (entre coma 2 y 3)
        String lat = datos.substring(comas[2] + 1, comas[3]);
        // Campo 4: Longitude (entre coma 3 y 4)
        String lon = datos.substring(comas[3] + 1, comas[4]);
        
        lat.trim();
        lon.trim();
        
        Serial.println(">> GNSS Run: " + gnssRun + ", Fix: " + fixStatus);
        Serial.println(">> Lat: '" + lat + "', Lon: '" + lon + "'");
        
        // Verificar que hay fix y coordenadas válidas
        if (fixStatus == "1" && lat.length() > 0 && lon.length() > 0 && 
            lat != "0.000000" && lon != "0.000000" &&
            lat != "" && lon != "") {
          Serial.println(">> ✓ Coordenadas obtenidas: " + lat + ", " + lon);
          return lat + "," + lon;
        } else {
          Serial.println(">> Fix no válido o coordenadas en 0");
        }
      }
    }
    
    Serial.println(">> Sin señal GPS. Esperando...");
    delay(3000);
  }
  
  Serial.println(">> No se pudo obtener ubicación GPS después de 5 intentos.");
  return "";
}

// ============================
// FUNCIONES HTTP
// ============================

bool inicializarHTTP() {
  Serial.println(">> Inicializando HTTP...");
  
  // Terminar cualquier sesión HTTP previa
  gsmSerial.println("AT+HTTPTERM");
  delay(500);
  
  // Inicializar HTTP
  gsmSerial.println("AT+HTTPINIT");
  delay(1000);
  String resp = esperarRespuesta();
  
  if (resp.indexOf("OK") == -1) {
    Serial.println(">> Error al inicializar HTTP");
    return false;
  }
  
  // Configurar CID (Context Identifier)
  gsmSerial.println("AT+HTTPPARA=\"CID\",1");
  delay(500);
  esperarRespuesta();
  
  return true;
}

bool enviarUbicacionHTTP(String coordenadas) {
  if (coordenadas.length() == 0) {
    Serial.println(">> No hay coordenadas para enviar");
    return false;
  }
  
  int comaPos = coordenadas.indexOf(',');
  if (comaPos == -1) return false;
  
  String lat = coordenadas.substring(0, comaPos);
  String lon = coordenadas.substring(comaPos + 1);
  
  Serial.println(">> Enviando ubicación al servidor...");
  
  if (!inicializarHTTP()) {
    return false;
  }
  
  // Construir URL con parámetros y token en query string
  // El SIM7600 no soporta headers personalizados fácilmente con AT+HTTPPARA
  String url = "http://" + String(API_ENDPOINT) + String(API_PATH) + 
               "?lat=" + lat + 
               "&lon=" + lon +
               "&token=" + String(DEVICE_TOKEN);  // Token como query param
  
  Serial.println(">> URL: " + url);
  
  gsmSerial.print("AT+HTTPPARA=\"URL\",\"");
  gsmSerial.print(url);
  gsmSerial.println("\"");
  delay(1000);
  esperarRespuesta();
  
  // Ejecutar petición GET (método 0)
  Serial.println(">> Ejecutando petición HTTP GET...");
  gsmSerial.println("AT+HTTPACTION=0");
  
  // Esperar respuesta del servidor (puede tardar)
  String respuesta = "";
  unsigned long timeout = millis() + 15000; // 15 segundos timeout
  
  while (millis() < timeout) {
    while (gsmSerial.available()) {
      char c = (char)gsmSerial.read();
      respuesta += c;
      Serial.print(c); // Imprimir en tiempo real
    }
    
    // Buscar la respuesta +HTTPACTION
    if (respuesta.indexOf("+HTTPACTION:") != -1) {
      break;
    }
    delay(100);
  }
  
  Serial.println(); // Nueva línea después de la respuesta
  
  // Verificar código de respuesta HTTP
  // Formato: +HTTPACTION: <Method>,<StatusCode>,<DataLen>
  // Ejemplo: +HTTPACTION: 0,200,47
  
  bool exito = false;
  
  if (respuesta.indexOf("+HTTPACTION: 0,200") != -1) {
    Serial.println(">> ✓ Ubicación enviada exitosamente (200 OK)");
    exito = true;
    
    // Leer respuesta del servidor
    delay(500);
    gsmSerial.println("AT+HTTPREAD");
    delay(2000);
    String contenido = esperarRespuesta(5000);
    Serial.println(">> Respuesta del servidor:");
    Serial.println(contenido);
    
  } else if (respuesta.indexOf("+HTTPACTION: 0,201") != -1) {
    Serial.println(">> ✓ Ubicación enviada exitosamente (201 Created)");
    exito = true;
    
  } else if (respuesta.indexOf("+HTTPACTION: 0,204") != -1) {
    Serial.println(">> ✓ Ubicación enviada exitosamente (204 No Content)");
    exito = true;
    
  } else if (respuesta.indexOf("+HTTPACTION: 0,") != -1) {
    // Extraer código de error
    int pos = respuesta.indexOf("+HTTPACTION: 0,");
    String codigo = respuesta.substring(pos + 15, pos + 18);
    Serial.println(">> ✗ Error HTTP " + codigo);
    
  } else if (respuesta.indexOf("ERROR") != -1) {
    Serial.println(">> ✗ Error en comando AT");
    
  } else {
    Serial.println(">> ✗ Timeout o respuesta no reconocida");
  }
  
  // Terminar sesión HTTP
  delay(500);
  gsmSerial.println("AT+HTTPTERM");
  delay(500);
  esperarRespuesta();
  
  return exito;
}

// ============================
// FUNCIONES DE CONECTIVIDAD
// ============================

bool verificarConexionGPRS() {
  Serial.println(">> Verificando conexión GPRS...");
  
  // Verificar registro en la red
  gsmSerial.println("AT+CREG?");
  delay(1000);
  String regResp = esperarRespuesta();
  Serial.println(">> Estado de registro: " + regResp);
  
  // Configurar APN para contexto PDP
  // Cambiar según tu operador
  Serial.println(">> Configurando APN...");
  gsmSerial.println("AT+CGDCONT=1,\"IP\",\"internet.itelcel.com\"");  // Cambiar APN aquí
  delay(1000);
  esperarRespuesta();
  
  // Activar contexto PDP
  Serial.println(">> Activando contexto PDP...");
  gsmSerial.println("AT+CGACT=1,1");
  delay(3000);
  String actResp = esperarRespuesta();
  
  // Verificar si ya está activo o si se activó correctamente
  if (actResp.indexOf("ERROR") != -1) {
    // Puede que ya esté activo, verificar estado
    gsmSerial.println("AT+CGACT?");
    delay(1000);
    String stateResp = esperarRespuesta();
    
    if (stateResp.indexOf("+CGACT: 1,1") != -1) {
      Serial.println(">> Contexto PDP ya estaba activo");
    } else {
      Serial.println(">> Error al activar contexto PDP");
      return false;
    }
  }
  
  // Obtener dirección IP asignada (opcional, para verificar)
  gsmSerial.println("AT+CGPADDR=1");
  delay(1000);
  String ipResp = esperarRespuesta();
  Serial.println(">> Dirección IP: " + ipResp);
  
  Serial.println(">> ✓ GPRS conectado y listo");
  return true;
}

// ============================
// SETUP Y LOOP
// ============================

void setup() {
  Serial.begin(BAUD_RATE);
  delay(2000);
  Serial.println("\n\n>> =============================");
  Serial.println(">> GPS Tracker - FindMe32");
  Serial.println(">> =============================");
  Serial.println(">> Device Token: " + String(DEVICE_TOKEN));
  Serial.println(">> API Endpoint: " + String(API_ENDPOINT) + String(API_PATH));
  Serial.println(">> Intervalo: 5 minutos");
  Serial.println(">> =============================\n");
  
  // Configurar pin de encendido del módulo GSM
  pinMode(PWR_PIN, OUTPUT);
  digitalWrite(PWR_PIN, LOW);
  delay(100);
  digitalWrite(PWR_PIN, HIGH);
  delay(2000);
  digitalWrite(PWR_PIN, LOW);
  
  // Inicializar Serial GSM
  gsmSerial.begin(BAUD_RATE, SERIAL_8N1, RXD1_PIN, TXD1_PIN);
  
  Serial.println(">> Esperando que el módulo GSM esté listo...");
  delay(10000);
  
  // Verificar comunicación
  for (int i = 0; i < 3; i++) {
    gsmSerial.println("AT");
    delay(1000);
    String resp = esperarRespuesta();
    if (resp.indexOf("OK") != -1) {
      Serial.println(">> Módulo GSM respondiendo");
      break;
    }
    delay(2000);
  }
  
  // Inicializar GPS
  inicializarGPS();
  
  // Verificar conexión GPRS
  verificarConexionGPRS();
  
  Serial.println("\n>> Sistema listo. Comenzando ciclo de envío...\n");
  ultimoEnvio = millis();
}

void loop() {
  unsigned long tiempoActual = millis();
  
  // Verificar si han pasado 5 minutos
  if (tiempoActual - ultimoEnvio >= INTERVALO_ENVIO) {
    Serial.println("\n>> ===== CICLO DE ENVÍO =====");
    Serial.print(">> Tiempo transcurrido: ");
    Serial.print((tiempoActual - ultimoEnvio) / 1000);
    Serial.println(" segundos");
    
    // Verificar conexión GPRS
    if (!verificarConexionGPRS()) {
      Serial.println(">> Error: Sin conexión GPRS. Reintentando en el próximo ciclo...");
      ultimoEnvio = tiempoActual;
      return;
    }
    
    // Obtener coordenadas GPS
    String coordenadas = obtenerCoordenadas();
    
    if (coordenadas.length() > 0) {
      // Enviar ubicación al servidor
      bool exito = enviarUbicacionHTTP(coordenadas);
      
      if (exito) {
        Serial.println(">> ✓ Ciclo completado exitosamente");
      } else {
        Serial.println(">> ✗ Error al enviar datos al servidor");
      }
    } else {
      Serial.println(">> ✗ No se pudo obtener ubicación GPS");
    }
    
    ultimoEnvio = tiempoActual;
    Serial.println(">> ============================\n");
  }
  
  // Pequeño delay para no saturar el loop
  delay(1000);
}
