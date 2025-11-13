#include "findme32httptest.h"

// APN y URL de prueba
static const char *APN = "internet.itelcel.com";
static const char *TEST_URL = "https://tunel.ponlecoco.com/api/gps/gpstracker?lat=19.432608&lon=-99.133209&token=dt_80ced305b98eaf5d73b39281db9a39529ad6523d46a839e3873c2d69fc896f43";

FindMe32HttpTest::FindMe32HttpTest(HardwareSerial &serial, int pwrPin_, int rxPin_, int txPin_, unsigned long baud)
  : gsm(serial), pwrPin(pwrPin_), rxPin(rxPin_), txPin(txPin_), baudRate(baud) {}

void FindMe32HttpTest::begin() {
  // Configurar pin de encendido y Serial1
  pinMode(pwrPin, OUTPUT);
  digitalWrite(pwrPin, LOW);
  delay(100);
  digitalWrite(pwrPin, HIGH);
  delay(2000);
  digitalWrite(pwrPin, LOW);

  gsm.begin(baudRate, SERIAL_8N1, rxPin, txPin);
  delay(2000);

  // Comprobar comunicación básica
  for (int i = 0; i < 3; i++) {
    gsm.println("AT");
    delay(500);
    String r = waitResponse(1000);
    if (r.indexOf("OK") != -1) break;
    delay(500);
  }
}

String FindMe32HttpTest::waitResponse(unsigned long timeout_ms) {
  String resp = "";
  unsigned long end = millis() + timeout_ms;
  while (millis() < end) {
    while (gsm.available()) {
      resp += (char)gsm.read();
    }
    if (resp.indexOf("OK") != -1 || resp.indexOf("ERROR") != -1 || resp.indexOf("+HTTPACTION:") != -1) {
      break;
    }
    delay(100);
  }
  return resp;
}

bool FindMe32HttpTest::ensureGPRS() {
  Serial.println(">> [HTTPTEST] Verificando GPRS...");

  // Verificar y sincronizar hora (CRÍTICO para SSL/TLS)
  Serial.println(">> [HTTPTEST] Verificando fecha/hora del módulo...");
  gsm.println("AT+CCLK?");
  delay(500);
  String reloj = waitResponse(2000);
  Serial.println(">> Fecha/Hora: " + reloj);
  
  // Si la fecha es inválida, configurar sincronización y reiniciar
  // Fecha válida debe ser >= 2020 ("20/ en adelante)
  bool fechaInvalida = false;
  if (reloj.indexOf("ERROR") != -1) {
    fechaInvalida = true;
  } else if (reloj.indexOf("+CCLK: \"") != -1) {
    int pos = reloj.indexOf("+CCLK: \"");
    if (pos != -1 && pos + 9 < reloj.length()) {
      String anio = reloj.substring(pos + 8, pos + 10);
      int anioNum = anio.toInt();
      if (anioNum < 20) {
        fechaInvalida = true;
        Serial.println(">> [HTTPTEST] Año: 20" + anio + " (inválido)");
      }
    }
  }
  
  if (fechaInvalida) {
    Serial.println(">> [HTTPTEST] Sincronizando fecha/hora (reinicio requerido)...");
    gsm.println("AT+CTZU=1");
    delay(500);
    waitResponse(1000);
    gsm.println("AT+CLTS=1");
    delay(500);
    waitResponse(1000);
    gsm.println("AT&W");
    delay(1000);
    waitResponse(2000);
    // Usar AT+CFUN=1,1 para reinicio completo (más profundo que AT+CRESET)
    gsm.println("AT+CFUN=1,1");
    Serial.println(">> [HTTPTEST] Reiniciando módulo, espera 25s...");
    delay(25000);
    
    // Re-verificar hora después del reinicio
    gsm.println("AT+CCLK?");
    delay(500);
    reloj = waitResponse(2000);
    Serial.println(">> Nueva Fecha/Hora: " + reloj);
    
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
          Serial.println(">> [HTTPTEST] Año post-reinicio: 20" + anio + " (inválido)");
        } else {
          Serial.println(">> [HTTPTEST] Año post-reinicio: 20" + anio + " (válido)");
        }
      }
    }
    
    if (fechaInvalida) {
      Serial.println(">> ✗ ADVERTENCIA: Reloj aún incorrecto, SSL puede fallar");
    } else {
      Serial.println(">> ✓ Reloj sincronizado correctamente");
    }
  }

  gsm.println("AT+CSQ");
  delay(500);
  Serial.println(">> CSQ: " + waitResponse(1000));

  gsm.println("AT+CREG?");
  delay(500);
  String reg = waitResponse(1000);
  Serial.println(">> CREG: " + reg);

  // Si no está registrado, seguir intentando brevemente
  if (reg.indexOf(",1") == -1 && reg.indexOf(",5") == -1) {
    Serial.println(">> No registrado, esperando 5s y reintentando...");
    delay(5000);
    gsm.println("AT+CREG?");
    delay(1000);
    reg = waitResponse(2000);
    Serial.println(">> CREG2: " + reg);
    if (reg.indexOf(",1") == -1 && reg.indexOf(",5") == -1) {
      Serial.println(">> ✗ No hay registro en red");
      return false;
    }
  }

  // Configurar APN
  gsm.print("AT+CGDCONT=1,\"IP\",\""); gsm.print(APN); gsm.println("\"");
  delay(1000);
  Serial.println(">> CGDCONT: " + waitResponse(2000));

  // Activar PDP
  gsm.println("AT+CGACT=1,1");
  delay(3000);
  String act = waitResponse(3000);
  Serial.println(">> CGACT: " + act);

  // Comprobar IP
  gsm.println("AT+CGPADDR=1");
  delay(1000);
  String ip = waitResponse(2000);
  Serial.println(">> CGPADDR: " + ip);
  if (ip.indexOf("0.0.0.0") != -1 || ip.indexOf("ERROR") != -1) return false;

  return true;
}

bool FindMe32HttpTest::runTest() {
  Serial.println(">> [HTTPTEST] Iniciando prueba HTTPS...");

  if (!ensureGPRS()) {
    Serial.println(">> ✗ GPRS no disponible, abortando prueba");
    return false;
  }

  // Terminar sesión previa
  gsm.println("AT+HTTPTERM");
  delay(500);
  waitResponse(1000);

  // Inicializar HTTP
  gsm.println("AT+HTTPINIT");
  delay(1000);
  String init = waitResponse(2000);
  Serial.println(">> HTTPINIT: " + init);
  if (init.indexOf("OK") == -1) {
    Serial.println(">> ✗ No se pudo inicializar HTTP");
    return false;
  }

  // CID
  gsm.println("AT+HTTPPARA=\"CID\",1");
  delay(500);
  waitResponse(1000);

  // Forzar SSL/TLS
  gsm.println("AT+HTTPSSL=1");
  delay(500);
  waitResponse(1000);

  // SSL config: TLS1.2 y no validar certificado (para túneles/entornos locales)
  gsm.println("AT+CSSLCFG=\"sslversion\",0,4");
  delay(300);
  waitResponse(500);
  gsm.println("AT+CSSLCFG=\"authmode\",0,0");
  delay(300);
  waitResponse(500);

  // URL
  gsm.print("AT+HTTPPARA=\"URL\",\"");
  gsm.print(TEST_URL);
  gsm.println("\"");
  delay(500);
  Serial.println(">> URL set: " + waitResponse(1000));

  // Acción GET
  gsm.println("AT+HTTPACTION=0");
  unsigned long end = millis() + 60000; // 60s timeout
  String resp = "";
  bool gotAction = false;
  while (millis() < end) {
    while (gsm.available()) {
      char c = (char)gsm.read();
      resp += c;
      Serial.print(c);
    }
    if (resp.indexOf("+HTTPACTION:") != -1) { gotAction = true; break; }
    if (resp.indexOf("ERROR") != -1) break;
    delay(100);
  }
  Serial.println();

  if (!gotAction) {
    Serial.println(">> ✗ No se recibió +HTTPACTION, respuesta: " + resp);
    gsm.println("AT+HTTPTERM");
    waitResponse(500);
    return false;
  }

  // Leer código
  int p = resp.indexOf("+HTTPACTION:");
  if (p != -1) {
    String after = resp.substring(p);
    // Formato: +HTTPACTION: 0,200,Length
    int c1 = after.indexOf(',');
    int c2 = after.indexOf(',', c1 + 1);
    if (c1 != -1 && c2 != -1) {
      String code = after.substring(c1 + 1, c2);
      code.trim();
      Serial.println(">> HTTP Code: " + code);
      if (code.startsWith("2")) {
        // Leer contenido
        gsm.println("AT+HTTPREAD");
        delay(2000);
        String content = waitResponse(5000);
        Serial.println(">> HTTP Content:\n" + content);
        gsm.println("AT+HTTPTERM");
        waitResponse(500);
        return true;
      } else {
        Serial.println(">> ✗ Código HTTP no exitoso: " + code);
      }
    }
  }

  gsm.println("AT+HTTPTERM");
  waitResponse(500);
  return false;
}
