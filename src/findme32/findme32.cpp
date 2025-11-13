#include <Arduino.h>

// ============================
// CONFIGURACIÓN DEL DISPOSITIVO
// ============================
#define DEVICE_TOKEN "dt_80ced305b98eaf5d73b39281db9a39529ad6523d46a839e3873c2d69fc896f43"  // Cambiar en cada dispositivo
#define API_ENDPOINT "tunel.ponlecoco.com"
#define API_PATH "/api/gps/gpstracker"
#define API_PORT "443"  // 443 para HTTPS

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
// PROTOTIPOS DE FUNCIONES
// ============================
void imprimirRespuesta();
String esperarRespuesta(unsigned long timeout_ms = 5000);
bool inicializarGPS();
String obtenerCoordenadas();
bool inicializarHTTP();
bool enviarUbicacionHTTP(String coordenadas);
bool verificarConexionGPRS();

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

String esperarRespuesta(unsigned long timeout_ms) {
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
  
  // Encender GPS/GNSS (comando correcto con doble S: CGNSSPWR)
  gsmSerial.println("AT+CGNSSPWR=1");
  delay(1000);
  
  String respuesta = "";
  while (gsmSerial.available()) {
    respuesta += (char)gsmSerial.read();
  }
  
  Serial.println(">> Respuesta encendido GPS: " + respuesta);
  
  if (respuesta.indexOf("OK") != -1) {
    Serial.println(">> ✓ GPS encendido correctamente");
  } else {
    Serial.println(">> Reintentando encendido GPS...");
    gsmSerial.println("AT+CGNSSPWR=1");
    delay(1000);
    while (gsmSerial.available()) {
      respuesta += (char)gsmSerial.read();
    }
    Serial.println(">> Respuesta reintento: " + respuesta);
  }
  
  Serial.println(">> GPS inicializado");
  return true;
}

String obtenerCoordenadas() {
  Serial.println(">> Obteniendo coordenadas GPS...");
  Serial.println(">> Esperando señal GPS (hasta 30 intentos)...");
  
  for (int intento = 1; intento <= 30; intento++) {
    Serial.print(">> Intento ");
    Serial.print(intento);
    Serial.println(" para obtener ubicación GPS...");
    
    gsmSerial.println("AT+CGNSSINFO");
    delay(2000);
    
    String respuesta = "";
    while (gsmSerial.available()) {
      respuesta += (char)gsmSerial.read();
    }
    respuesta.trim();
    
    Serial.println(">> Respuesta GPS: " + respuesta);
    
    // Formato: +CGNSSINFO: <mode>,<GPS satellites>,<GLONASS satellites>,<BEIDOU satellites>,
    // <reserved>,<Latitude>,<N/S>,<Longitude>,<E/W>,<date>,<UTC time>,<altitude>,<speed>,<course>,<PDOP>,<HDOP>,<VDOP>,<GPS satellites in view>
    // Ejemplo: +CGNSSINFO: 3,08,,02,00,18.9104824,N,99.2066956,W,131125,053852.00,1409.4,0.000,,20.97,8.57,19.14,04
    
    if (respuesta.indexOf("+CGNSSINFO:") != -1) {
      int inicio = respuesta.indexOf("+CGNSSINFO:");
      int finLinea = respuesta.indexOf('\n', inicio);
      if (finLinea == -1) finLinea = respuesta.length();
      
      String linea = respuesta.substring(inicio + 12, finLinea); // Saltar "+CGNSSINFO: "
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
      
      if (campoCount >= 9) {
        // Campo 5: Latitude (después de la 5ta coma)
        String lat = linea.substring(campos[5] + 1, campos[6]);
        // Campo 6: N/S
        String latDir = linea.substring(campos[6] + 1, campos[7]);
        // Campo 7: Longitude
        String lon = linea.substring(campos[7] + 1, campos[8]);
        // Campo 8: E/W
        String lonDir = linea.substring(campos[8] + 1, campos[9]);
        
        lat.trim();
        latDir.trim();
        lon.trim();
        lonDir.trim();
        
        Serial.println(">> Latitud: " + lat + " " + latDir);
        Serial.println(">> Longitud: " + lon + " " + lonDir);
        
        // Verificar coordenadas válidas
        if (lat.length() > 0 && lon.length() > 0 && 
            lat != "" && lon != "" &&
            lat != "0" && lon != "0") {
          
          // Convertir a decimal con signo
          float latNum = lat.toFloat();
          float lonNum = lon.toFloat();
          
          // Aplicar dirección (S y W son negativos)
          if (latDir == "S") latNum = -latNum;
          if (lonDir == "W") lonNum = -lonNum;
          
          String coordenadas = String(latNum, 6) + "," + String(lonNum, 6);
          Serial.println(">> ✓ Coordenadas obtenidas: " + coordenadas);
          Serial.println(">> Link: https://maps.google.com/?q=" + coordenadas);
          
          return coordenadas;
        } else {
          Serial.println(">> Coordenadas vacías o en cero");
        }
      }
    }
    
    Serial.println(">> No se obtuvo ubicación válida. Reintentando...");
    delay(3000);
  }
  
  Serial.println(">> No se pudo obtener ubicación GPS después de 30 intentos.");
  return "";
}

// ============================
// FUNCIONES HTTP
// ============================

bool inicializarHTTP() {
  Serial.println(">> Inicializando HTTPS...");
  
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
  
  // Habilitar SSL/TLS para HTTPS
  Serial.println(">> Habilitando SSL/TLS...");
  gsmSerial.println("AT+HTTPSSL=1");
  delay(500);
  esperarRespuesta();
  
  // Configurar SSL para aceptar cualquier certificado (necesario para algunos servidores)
  Serial.println(">> Configurando validación SSL...");
  gsmSerial.println("AT+CSSLCFG=\"sslversion\",0,4"); // TLS 1.2
  delay(500);
  esperarRespuesta();
  
  gsmSerial.println("AT+CSSLCFG=\"authmode\",0,0"); // No verificar certificado del servidor
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
  
  // Verificar que el contexto PDP está activo antes de HTTP
  Serial.println(">> Verificando contexto PDP...");
  gsmSerial.println("AT+CGACT?");
  delay(1000);
  String pdpStatus = esperarRespuesta();
  Serial.println(">> Estado PDP: " + pdpStatus);
  
  if (pdpStatus.indexOf("+CGACT: 1,1") == -1) {
    Serial.println(">> Contexto PDP inactivo. Reactivando...");
    if (!verificarConexionGPRS()) {
      Serial.println(">> Error: No se pudo reactivar GPRS");
      return false;
    }
  }
  
  if (!inicializarHTTP()) {
    return false;
  }
  
  // Construir URL con parámetros y token en query string
  String url = "https://" + String(API_ENDPOINT) + String(API_PATH) + 
               "?lat=" + lat + 
               "&lon=" + lon +
               "&token=" + String(DEVICE_TOKEN);
  
  Serial.println(">> URL: " + url);
  
  gsmSerial.print("AT+HTTPPARA=\"URL\",\"");
  gsmSerial.print(url);
  gsmSerial.println("\"");
  delay(1000);
  String urlResp = esperarRespuesta();
  Serial.println(">> URL Config: " + urlResp);
  
  // Ejecutar petición GET (método 0)
  Serial.println(">> Ejecutando petición HTTP GET...");
  gsmSerial.println("AT+HTTPACTION=0");
  
  // Esperar respuesta del servidor con timeout más largo para HTTPS
  String respuesta = "";
  unsigned long timeout = millis() + 60000; // 60 segundos para HTTPS con SSL
  bool httpActionRecibido = false;
  
  while (millis() < timeout) {
    while (gsmSerial.available()) {
      char c = (char)gsmSerial.read();
      respuesta += c;
      Serial.print(c); // Imprimir en tiempo real
    }
    
    // Buscar la respuesta +HTTPACTION o errores
    if (respuesta.indexOf("+HTTPACTION:") != -1) {
      httpActionRecibido = true;
      break;
    }
    
    // Detectar errores de red
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
  
  Serial.println(); // Nueva línea después de la respuesta
  
  // Si hubo error de red, intentar reconectar
  if (!httpActionRecibido && (respuesta.indexOf("+HTTP_NONET_EVENT") != -1 || 
      respuesta.indexOf("+CGEV: NW PDN DEACT") != -1)) {
    Serial.println(">> Detectado error de red. Terminando HTTP y saliendo...");
    gsmSerial.println("AT+HTTPTERM");
    delay(500);
    esperarRespuesta();
    return false;
  }
  
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
    
    // Errores comunes del SIM7600
    if (codigo == "715") {
      Serial.println(">> ERROR 715: Timeout SSL/TLS o certificado inválido");
      Serial.println(">> Sugerencias:");
      Serial.println(">>   - Verificar que el servidor soporte TLS 1.2");
      Serial.println(">>   - Verificar fecha/hora del módulo (AT+CCLK?)");
      Serial.println(">>   - Verificar que la señal sea estable (+CSQ)");
    } else if (codigo == "703") {
      Serial.println(">> ERROR 703: Error de DNS");
    } else if (codigo == "714") {
      Serial.println(">> ERROR 714: Timeout HTTP");
    }
    
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
  
  // Verificar calidad de señal
  gsmSerial.println("AT+CSQ");
  delay(500);
  String csqResp = esperarRespuesta();
  Serial.println(">> Calidad de señal: " + csqResp);
  
  // Verificar registro en la red
  gsmSerial.println("AT+CREG?");
  delay(1000);
  String regResp = esperarRespuesta();
  Serial.println(">> Estado de registro: " + regResp);
  
  // Verificar si ya hay un contexto PDP activo
  gsmSerial.println("AT+CGACT?");
  delay(1000);
  String cgactCheck = esperarRespuesta();
  Serial.println(">> Estado actual PDP: " + cgactCheck);
  
  // Si ya está activo, no hacer nada más
  if (cgactCheck.indexOf("+CGACT: 1,1") != -1) {
    Serial.println(">> ✓ Contexto PDP ya está activo");
    return true;
  }
  
  // Configurar APN para contexto PDP - Telcel
  Serial.println(">> Configurando APN Telcel...");
  gsmSerial.println("AT+CGDCONT=1,\"IP\",\"internet.itelcel.com\"");
  delay(1000);
  String cgdcontResp = esperarRespuesta();
  Serial.println(">> Configuración APN: " + cgdcontResp);
  
  // Activar contexto PDP
  Serial.println(">> Activando contexto PDP...");
  gsmSerial.println("AT+CGACT=1,1");
  delay(5000); // Esperar más tiempo para la activación
  String actResp = esperarRespuesta();
  
  Serial.println(">> Respuesta activación: " + actResp);
  
  // Verificar si se activó correctamente
  if (actResp.indexOf("ERROR") != -1) {
    // Puede que ya esté activo, verificar estado
    Serial.println(">> Error en activación, verificando estado...");
    gsmSerial.println("AT+CGACT?");
    delay(1000);
    String stateResp = esperarRespuesta();
    
    if (stateResp.indexOf("+CGACT: 1,1") != -1) {
      Serial.println(">> Contexto PDP ya estaba activo");
    } else {
      Serial.println(">> ✗ Error: No se pudo activar contexto PDP");
      return false;
    }
  }
  
  // Obtener dirección IP asignada para confirmar
  delay(2000); // Esperar a que se asigne IP
  gsmSerial.println("AT+CGPADDR=1");
  delay(1000);
  String ipResp = esperarRespuesta();
  Serial.println(">> Dirección IP asignada: " + ipResp);
  
  if (ipResp.indexOf("ERROR") != -1 || ipResp.indexOf("0.0.0.0") != -1) {
    Serial.println(">> ✗ Error: No se obtuvo dirección IP válida");
    return false;
  }
  
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
  
  // Verificar y esperar registro en la red
  Serial.println(">> Verificando registro en la red...");
  bool registrado = false;
  
  for (int intento = 1; intento <= 30; intento++) {
    gsmSerial.println("AT+CREG?");
    delay(1000);
    String regResp = esperarRespuesta();
    Serial.println(">> CREG: " + regResp);
    
    // +CREG: 0,1 o +CREG: 1,1 = registrado en red local
    // +CREG: 0,5 o +CREG: 1,5 = registrado en roaming
    if (regResp.indexOf(",1") != -1 || regResp.indexOf(",5") != -1) {
      Serial.println(">> ✓ Módulo registrado en la red");
      registrado = true;
      break;
    }
    
    Serial.print(">> Esperando registro en la red... Intento ");
    Serial.print(intento);
    Serial.println(" de 30");
    delay(2000);
  }
  
  if (!registrado) {
    Serial.println(">> ✗ ADVERTENCIA: No se pudo registrar en la red");
    Serial.println(">> El LED verde debería parpadear lento cuando esté registrado");
  }
  
  // Verificar señal
  gsmSerial.println("AT+CSQ");
  delay(500);
  String csq = esperarRespuesta();
  Serial.println(">> Calidad de señal: " + csq);
  
  // Verificar fecha/hora del módulo (importante para SSL/TLS)
  Serial.println(">> Verificando fecha/hora del módulo...");
  gsmSerial.println("AT+CCLK?");
  delay(500);
  String reloj = esperarRespuesta();
  Serial.println(">> Fecha/Hora: " + reloj);
  
  // Si la fecha es inválida, intentar sincronizar con la red
  // Fecha válida debe ser 2025 = "25/... en adelante
  // Detectar fechas inválidas: año < 2020 (menos de "20/)
  bool fechaInvalida = false;
  if (reloj.indexOf("ERROR") != -1) {
    fechaInvalida = true;
  } else if (reloj.indexOf("+CCLK: \"") != -1) {
    // Extraer los primeros 2 dígitos del año (formato: "YY/MM/DD...)
    int pos = reloj.indexOf("+CCLK: \"");
    if (pos != -1 && pos + 9 < reloj.length()) {
      String anio = reloj.substring(pos + 8, pos + 10);
      int anioNum = anio.toInt();
      // Año debe ser >= 20 (2020 o superior)
      if (anioNum < 20) {
        fechaInvalida = true;
        Serial.println(">> Año detectado: 20" + anio + " (inválido, debe ser >= 2020)");
      }
    }
  }
  
  if (fechaInvalida) {
    Serial.println(">> Sincronizando fecha/hora con la red (requiere reinicio)...");
    gsmSerial.println("AT+CTZU=1"); // Auto-actualizar zona horaria
    delay(500);
    esperarRespuesta();
    
    gsmSerial.println("AT+CLTS=1"); // Habilitar sincronización de tiempo
    delay(500);
    esperarRespuesta();
    
    Serial.println(">> Guardando configuración (AT&W) y reiniciando (AT+CFUN=1,1)...");
    
    gsmSerial.println("AT&W"); // Guardar la configuración (incluyendo CLTS)
    delay(1000);
    esperarRespuesta();
    
    // Usar AT+CFUN=1,1 para un reinicio completo (más profundo que AT+CRESET)
    gsmSerial.println("AT+CFUN=1,1"); 
    
    Serial.println(">> Módulo reiniciando. Esperando 25 segundos...");
    delay(25000); // Esperar a que el módulo reinicie completamente
    
    // Volver a verificar el registro en la red (CRUCIAL después de reiniciar)
    registrado = false; // Forzar verificación de nuevo
    Serial.println(">> Verificando registro en la red (Post-Reinicio)...");
    for (int intento = 1; intento <= 30; intento++) {
      gsmSerial.println("AT+CREG?");
      delay(1000);
      String regResp = esperarRespuesta();
      Serial.println(">> CREG: " + regResp);
      
      if (regResp.indexOf(",1") != -1 || regResp.indexOf(",5") != -1) {
        Serial.println(">> ✓ Módulo registrado en la red (Post-Reinicio)");
        registrado = true;
        break;
      }
      Serial.print(">> Esperando registro... Intento ");
      Serial.println(intento);
      delay(2000);
    }
    
    // Verificar la hora OTRA VEZ
    gsmSerial.println("AT+CCLK?");
    delay(500);
    reloj = esperarRespuesta();
    Serial.println(">> Nueva Fecha/Hora (Post-Reinicio): " + reloj);
    
    // Verificar si sigue siendo inválida
    fechaInvalida = false;
    if (reloj.indexOf("ERROR") != -1) {
      fechaInvalida = true;
    } else if (reloj.indexOf("+CCLK: \"") != -1) {
      int pos = reloj.indexOf("+CCLK: \"");
      if (pos != -1 && pos + 9 < reloj.length()) {
        String anio = reloj.substring(pos + 8, pos + 10);
        int anioNum = anio.toInt();
        if (anioNum < 20) {
          fechaInvalida = true;
          Serial.println(">> Año post-reinicio: 20" + anio + " (aún inválido)");
        } else {
          Serial.println(">> Año post-reinicio: 20" + anio + " (válido)");
        }
      }
    }
    
    if (fechaInvalida) {
      Serial.println(">> ✗ ADVERTENCIA: El reloj sigue incorrecto. SSL fallará.");
    } else {
      Serial.println(">> ✓ Reloj sincronizado correctamente.");
    }
  }
  
  // Inicializar GPS
  inicializarGPS();
  
  // Verificar conexión GPRS
  verificarConexionGPRS();
  
  Serial.println("\n>> Sistema listo. Comenzando ciclo de envío...\n");
  
  // Hacer el primer envío inmediatamente
  Serial.println("\n>> ===== PRIMER ENVÍO (Inmediato) =====");
  String coordenadas = obtenerCoordenadas();
  if (coordenadas.length() > 0) {
    bool exito = enviarUbicacionHTTP(coordenadas);
    if (exito) {
      Serial.println(">> ✓ Primer envío completado exitosamente");
    } else {
      Serial.println(">> ✗ Error en el primer envío");
    }
  } else {
    Serial.println(">> ✗ No se pudo obtener ubicación GPS en el primer intento");
  }
  Serial.println(">> ============================\n");
  
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
