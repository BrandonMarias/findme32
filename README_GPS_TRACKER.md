# FindMe32 - GPS Tracker

Sistema de rastreo GPS que envía ubicaciones cada 5 minutos a un servidor backend.

## 📋 Características

- ✅ Envío automático de coordenadas GPS cada 5 minutos
- ✅ Autenticación mediante token único por dispositivo
- ✅ Conexión HTTP/HTTPS al backend
- ✅ Manejo de errores y reintentos
- ✅ Logs detallados por Serial Monitor

## 🔧 Configuración del Dispositivo ESP32

### 1. Editar el Token del Dispositivo

En `src/findme32/findme32.cpp`, cambiar:

```cpp
#define DEVICE_TOKEN "TU_TOKEN_UNICO_AQUI"  // ⚠️ Cambiar en cada dispositivo
```

**Token actual configurado:**
```cpp
#define DEVICE_TOKEN "dt_80ced305b98eaf5d73b39281db9a39529ad6523d46a839e3873c2d69fc896f43"
```

**Para múltiples dispositivos, usar tokens diferentes:**
- Dispositivo 1: `"dt_80ced305b98eaf5d73b39281db9a39529ad6523d46a839e3873c2d69fc896f43"`
- Dispositivo 2: `"dt_otro_token_unico_para_dispositivo_2"`
- Dispositivo 3: `"dt_otro_token_unico_para_dispositivo_3"`

### 2. Configurar el Endpoint del Backend

```cpp
#define API_ENDPOINT "tunel.ponlecoco.com"     // Tu dominio
#define API_PATH "/api/gps/gpstracker"         // Ruta del endpoint
#define API_PORT "443"                          // 443 para HTTPS
```

### 3. Configurar el APN de tu Operador

El código está configurado para **Telcel** con el APN:

```cpp
gsmSerial.println("AT+CGDCONT=1,\"IP\",\"internet.itelcel.com\"");
```

**Otros APNs comunes en México:**
- Telcel: `"internet.itelcel.com"` ✅ (Configurado)
- Movistar: `"internet.movistar.com.mx"`
- AT&T: `"internet.att.com.mx"`
- Unefon: `"internet.unefon.com.mx"`

## 📡 Comandos AT Utilizados

### GPS/GNSS
- `AT+CGNSPWR=1` - Encender GPS
- `AT+CGNSINF` - Obtener información de ubicación (coordenadas, fix status, etc.)

### HTTPS/HTTP
- `AT+HTTPINIT` - Inicializar servicio HTTP
- `AT+HTTPTERM` - Terminar sesión HTTP
- `AT+HTTPPARA="CID",1` - Configurar contexto PDP
- `AT+HTTPPARA="URL","..."` - Configurar URL completa con query params
- `AT+HTTPSSL=1` - Habilitar SSL/TLS para HTTPS
- `AT+HTTPACTION=0` - Ejecutar petición GET (0=GET, 1=POST, 2=HEAD)
- `AT+HTTPREAD` - Leer respuesta del servidor

### GPRS (SIM7600)
- `AT+CREG?` - Verificar registro en la red
- `AT+CGDCONT=1,"IP","internet.itelcel.com"` - Configurar APN Telcel
- `AT+CGACT=1,1` - Activar contexto PDP
- `AT+CGPADDR=1` - Obtener dirección IP asignada

## 🚀 Compilación y Carga

### PlatformIO (Recomendado)

```bash
# Compilar
pio run

# Cargar al dispositivo
pio run --target upload

# Monitor serial
pio device monitor
```

### Cambiar el directorio fuente

En `platformio.ini`:
```ini
[platformio]
src_dir = src/findme32  # Para GPS tracker
# src_dir = src/main    # Para control por SMS
```

## 📊 Monitoreo

El sistema imprime logs detallados por el puerto serial (115200 baud):

```
>> =============================
>> GPS Tracker - FindMe32
>> =============================
>> Device Token: dt_80ced305b98eaf5d73b39281db9a39529ad6523d46a839e3873c2d69fc896f43
>> API Endpoint: tunel.ponlecoco.com/api/gps/gpstracker
>> Intervalo: 5 minutos
>> =============================

>> Esperando que el módulo GSM esté listo...
>> Módulo GSM respondiendo
>> Inicializando GPS...
>> Verificando conexión GPRS...
>> Configurando APN Telcel...
>> Activando contexto PDP...
>> Dirección IP: +CGPADDR: 1,"10.xxx.xxx.xxx"
>> ✓ GPRS conectado y listo

>> Sistema listo. Comenzando ciclo de envío...

>> ===== CICLO DE ENVÍO =====
>> Tiempo transcurrido: 300 segundos
>> Obteniendo coordenadas GPS...
>> Intento 1 de 5
>> Respuesta GPS: +CGNSINF: 1,1,20251112120000.000,19.432608,-99.133209,525.4,...
>> GNSS Run: 1, Fix: 1
>> Lat: '19.432608', Lon: '-99.133209'
>> ✓ Coordenadas obtenidas: 19.432608, -99.133209
>> Enviando ubicación al servidor...
>> Inicializando HTTPS...
>> Habilitando SSL/TLS...
>> URL: https://tunel.ponlecoco.com/api/gps/gpstracker?lat=19.432608&lon=-99.133209&token=dt_80ced...
>> Ejecutando petición HTTP GET...
+HTTPACTION: 0,200,47
>> ✓ Ubicación enviada exitosamente (200 OK)
>> Respuesta del servidor:
+HTTPREAD: 47
{"success":true,"message":"Location received"}
>> ✓ Ciclo completado exitosamente
>> ============================
```

## 🌐 Backend NestJS

Ver archivo `BACKEND_NESTJS_CONFIG.md` para:
- Configuración CORS completa
- Controladores y servicios
- Esquema de base de datos
- Validación de tokens
- Rate limiting
- Ejemplos de código completo

### Endpoint esperado:

**GET** `https://tunel.ponlecoco.com/api/gps/gpstracker?lat=19.432608&lon=-99.133209&token=dt_80ced305b98eaf5d73b39281db9a39529ad6523d46a839e3873c2d69fc896f43`

**Query Parameters:**
```
lat: Latitud en grados decimales
lon: Longitud en grados decimales
token: Token único del dispositivo
```

**Respuesta esperada:**
```json
{
  "success": true,
  "message": "Location received successfully",
  "data": {
    "id": "uuid",
    "deviceToken": "ESP32_TRACKER_001",
    "latitude": 19.432608,
    "longitude": -99.133209,
    "timestamp": "2025-11-09T12:00:00.000Z"
  }
}
```

## 🔒 Seguridad

1. **Token único por dispositivo**: Cada ESP32 debe tener un token diferente
2. **Validación en backend**: Implementar whitelist de tokens válidos
3. **HTTPS habilitado**: ✅ Usando SSL/TLS con `AT+HTTPSSL=1`
4. **Conexión segura**: ✅ Puerto 443 configurado
5. **Rate limiting**: Limitar peticiones por dispositivo en el backend
6. **Logs de auditoría**: Registrar todas las peticiones con timestamp
7. **Token seguro**: Usar tokens largos y aleatorios (como el configurado)

## 🛠️ Solución de Problemas

### GPS no obtiene coordenadas
- Asegurarse de estar en un lugar con visibilidad al cielo
- El primer fix puede tomar 1-2 minutos
- Verificar que la antena GPS esté conectada

### Error de conexión HTTP
- Verificar que el APN sea correcto
- Comprobar que hay señal GSM/3G/4G
- Revisar que el endpoint esté accesible desde internet

### Módulo GSM no responde
- Verificar conexiones de TX/RX
- Revisar alimentación (el módulo necesita 2A mínimo)
- Presionar el botón PWR manualmente si es necesario

## 📝 Notas Adicionales

- **Consumo de batería**: El GPS consume bastante energía. Considerar apagarlo entre lecturas.
- **Intervalo configurable**: Cambiar `INTERVALO_ENVIO` para ajustar la frecuencia de envío
- **Almacenamiento local**: Se puede agregar SD card para guardar ubicaciones cuando no hay conexión
- **Modo sleep**: Implementar deep sleep entre ciclos para ahorrar batería

## 📄 Licencia

Este proyecto es de código abierto. Úsalo libremente.

## 👨‍💻 Autor

Brandon - FindMe32 Project
