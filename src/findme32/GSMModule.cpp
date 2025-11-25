#include "GSMModule.h"
#include "config.h"

GSMModule::GSMModule(HardwareSerial& serial, int pwrPin_, int rxPin_, int txPin_, unsigned long baudRate_)
  : gsm(serial), pwrPin(pwrPin_), rxPin(rxPin_), txPin(txPin_), baudRate(baudRate_) {}

void GSMModule::begin() {
  encenderModulo();
  gsm.begin(baudRate, SERIAL_8N1, rxPin, txPin);
  
  Serial.println(">> Esperando que el módulo GSM esté listo...");
  delay(10000);
  
  verificarComunicacion();
}

void GSMModule::encenderModulo() {
  pinMode(pwrPin, OUTPUT);
  digitalWrite(pwrPin, LOW);
  delay(100);
  digitalWrite(pwrPin, HIGH);
  delay(2000);
  digitalWrite(pwrPin, LOW);
}

bool GSMModule::verificarComunicacion() {
  for (int i = 0; i < 3; i++) {
    gsm.println("AT");
    delay(1000);
    String resp = esperarRespuesta();
    if (resp.indexOf("OK") != -1) {
      Serial.println(">> Módulo GSM respondiendo");
      return true;
    }
    delay(2000);
  }
  return false;
}

String GSMModule::esperarRespuesta(unsigned long timeout_ms) {
  String respuesta = "";
  unsigned long timeout = millis() + timeout_ms;
  
  while (millis() < timeout) {
    while (gsm.available()) {
      respuesta += (char)gsm.read();
    }
    if (respuesta.indexOf("OK") != -1 || respuesta.indexOf("ERROR") != -1) {
      break;
    }
    delay(100);
  }
  
  return respuesta;
}

bool GSMModule::esperarRegistroRed(int maxIntentos) {
  Serial.println(">> Verificando registro en la red...");
  
  for (int intento = 1; intento <= maxIntentos; intento++) {
    gsm.println("AT+CREG?");
    delay(1000);
    String regResp = esperarRespuesta();
    Serial.println(">> CREG: " + regResp);
    
    if (regResp.indexOf(",1") != -1 || regResp.indexOf(",5") != -1) {
      Serial.println(">> ✓ Módulo registrado en la red");
      return true;
    }
    
    Serial.print(">> Esperando registro en la red... Intento ");
    Serial.print(intento);
    Serial.print(" de ");
    Serial.println(maxIntentos);
    delay(2000);
  }
  
  Serial.println(">> ✗ ADVERTENCIA: No se pudo registrar en la red");
  return false;
}

void GSMModule::verificarCalidadSenal() {
  gsm.println("AT+CSQ");
  delay(500);
  String csq = esperarRespuesta();
  Serial.println(">> Calidad de señal: " + csq);
}

bool GSMModule::necesitaSincronizarReloj(const String& reloj) {
  return (reloj.indexOf("ERROR") != -1 || 
          reloj.indexOf("70/") != -1 || 
          reloj.indexOf("80/") != -1 || 
          reloj.indexOf("00/") != -1);
}

bool GSMModule::verificarYSincronizarReloj() {
  Serial.println(">> Verificando fecha/hora del módulo...");
  gsm.println("AT+CCLK?");
  delay(500);
  String reloj = esperarRespuesta();
  Serial.println(">> Fecha/Hora: " + reloj);
  
  if (!necesitaSincronizarReloj(reloj)) {
    Serial.println(">> ✓ Reloj con fecha válida");
    return true;
  }
  
  Serial.println(">> Sincronizando fecha/hora con la red (requiere reinicio)...");
  
  gsm.println("AT+CTZU=1");
  delay(500);
  esperarRespuesta();
  
  gsm.println("AT+CLTS=1");
  delay(500);
  esperarRespuesta();
  
  Serial.println(">> Guardando configuración (AT&W) y reiniciando (AT+CFUN=1,1)...");
  
  gsm.println("AT&W");
  delay(1000);
  esperarRespuesta();
  
  gsm.println("AT+CFUN=1,1");
  
  Serial.println(">> Módulo reiniciando. Esperando 25 segundos...");
  delay(25000);
  
  // Volver a verificar registro
  Serial.println(">> Verificando registro en la red (Post-Reinicio)...");
  bool registrado = esperarRegistroRed(NETWORK_REGISTER_TIMEOUT);
  
  // Verificar la hora otra vez
  gsm.println("AT+CCLK?");
  delay(500);
  reloj = esperarRespuesta();
  Serial.println(">> Nueva Fecha/Hora (Post-Reinicio): " + reloj);
  
  if (necesitaSincronizarReloj(reloj)) {
    Serial.println(">> ✗ ADVERTENCIA: El reloj sigue incorrecto. SSL fallará.");
    return false;
  } else {
    Serial.println(">> ✓ Reloj sincronizado correctamente.");
    return true;
  }
}

bool GSMModule::estaContextoPDPActivo() {
  gsm.println("AT+CGACT?");
  delay(1000);
  String cgactCheck = esperarRespuesta();
  return (cgactCheck.indexOf("+CGACT: 1,1") != -1);
}

bool GSMModule::verificarConexionGPRS() {
  Serial.println(">> Verificando conexión GPRS...");
  
  verificarCalidadSenal();
  
  gsm.println("AT+CREG?");
  delay(1000);
  String regResp = esperarRespuesta();
  Serial.println(">> Estado de registro: " + regResp);
  
  gsm.println("AT+CGACT?");
  delay(1000);
  String cgactCheck = esperarRespuesta();
  Serial.println(">> Estado actual PDP: " + cgactCheck);
  
  if (cgactCheck.indexOf("+CGACT: 1,1") != -1) {
    Serial.println(">> ✓ Contexto PDP ya está activo");
    return true;
  }
  
  Serial.println(">> Configurando APN Telcel...");
  gsm.print("AT+CGDCONT=1,\"IP\",\"");
  gsm.print(APN_TELCEL);
  gsm.println("\"");
  delay(1000);
  String cgdcontResp = esperarRespuesta();
  Serial.println(">> Configuración APN: " + cgdcontResp);
  
  Serial.println(">> Activando contexto PDP...");
  gsm.println("AT+CGACT=1,1");
  delay(5000);
  String actResp = esperarRespuesta();
  
  Serial.println(">> Respuesta activación: " + actResp);
  
  if (actResp.indexOf("ERROR") != -1) {
    Serial.println(">> Error en activación, verificando estado...");
    gsm.println("AT+CGACT?");
    delay(1000);
    String stateResp = esperarRespuesta();
    
    if (stateResp.indexOf("+CGACT: 1,1") != -1) {
      Serial.println(">> Contexto PDP ya estaba activo");
    } else {
      Serial.println(">> ✗ Error: No se pudo activar contexto PDP");
      return false;
    }
  }
  
  delay(2000);
  gsm.println("AT+CGPADDR=1");
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

void GSMModule::reiniciarModulo() {
  Serial.println(">> Reiniciando módulo A7670SA completamente...");
  
  // Apagar módulo con pulso en PWR_PIN
  digitalWrite(pwrPin, HIGH);
  delay(3000);  // Mantener presionado 3 segundos para apagado
  digitalWrite(pwrPin, LOW);
  
  Serial.println(">> Módulo apagado. Esperando 5 segundos...");
  delay(5000);
  
  // Encender módulo nuevamente
  encenderModulo();
  
  Serial.println(">> Esperando que el módulo se inicialice...");
  delay(10000);
  
  // Verificar comunicación
  if (verificarComunicacion()) {
    Serial.println(">> ✓ Módulo reiniciado correctamente");
  } else {
    Serial.println(">> ✗ Advertencia: Módulo no responde después del reinicio");
  }
}
