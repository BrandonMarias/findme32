# FindMe32 - GPS Tracker System

Sistema de rastreo GPS profesional basado en ESP32-C3 y módulo A7670SA GSM/GPRS/GPS, con arquitectura modular y capacidades de control remoto.

## Descripción

FindMe32 es un sistema integral de rastreo vehicular que combina conectividad celular 4G LTE con GPS de alta precisión. El sistema ofrece dos modos de operación independientes:

- **Modo GPS Tracker**: Envío automático de ubicaciones con detección de movimiento
- **Modo Control SMS**: Control remoto mediante comandos de texto

## Características Principales

### Módulo GPS Tracker (findme32)

- Detección inteligente de movimiento con umbral configurable
- Cálculo automático de velocidad en km/h
- Transmisión periódica de ubicación (heartbeat)
- Comunicación segura HTTPS con SSL/TLS
- Soporte para túneles Cloudflare mediante SNI
- Sincronización automática de reloj con red celular
- Control de pines según estado del dispositivo
- Reinicio automático del GPS tras fallos consecutivos
- Arquitectura modular y escalable

### Módulo Control SMS (findme)

- Comandos de control remoto por SMS
- Autenticación mediante lista blanca de números
- Localización GPS bajo demanda
- Control de relevadores/actuadores
- Respuestas automáticas al remitente

## Arquitectura del Sistema

### Estructura de Directorios

```
src/
├── findme32/                    # Módulo GPS Tracker
│   ├── config.h                 # Configuración (no versionado)
│   ├── config_template.h        # Plantilla de configuración
│   ├── GSMModule.h/cpp          # Gestión del módulo GSM/GPRS
│   ├── GPSModule.h/cpp          # Control y parseo del GPS
│   ├── HTTPClient.h/cpp         # Cliente HTTPS
│   ├── GeoUtils.h/cpp           # Cálculos geográficos
│   └── findme32.cpp             # Programa principal
│
└── findme/                      # Módulo control SMS
    ├── findme_config.h          # Números autorizados (no versionado)
    ├── findme_config_template.h # Plantilla de números
    └── findme.cpp               # Programa de control por SMS
```

### Componentes Modulares

#### GSMModule
Gestiona todas las operaciones del módem celular:
- Inicialización con control de alimentación
- Registro en red con reintentos configurables
- Sincronización de reloj mediante AT+CFUN=1,1
- Configuración y activación de contextos PDP/GPRS
- Monitoreo de calidad de señal

#### GPSModule
Controla el subsistema GPS:
- Activación/desactivación del receptor GPS
- Parseo de coordenadas con validación
- Conversión automática de direcciones cardinales
- Reintentos configurables para obtención de fix
- Estructura de datos `GpsData` para coordenadas validadas

#### HTTPClient
Cliente HTTP/HTTPS con características avanzadas:
- Soporte completo para TLS 1.2
- Server Name Indication (SNI) para CDN
- Construcción dinámica de URLs con parámetros
- Parseo inteligente de respuestas HTTP
- Manejo robusto de errores (715, 703, 714)

#### GeoUtils
Utilidades para cálculos geográficos:
- Fórmula de Haversine para distancias
- Precisión en metros
- Optimizado para microcontroladores

## Hardware

### Componentes Principales

**Microcontrolador:**
- ESP32-C3-DevKitM-1

**Módulo Celular/GPS:**
- SIMCOM A7670SA


### Conexiones de Hardware

```
ESP32-C3          A7670SA
--------          --------
GPIO 10    --->   PWR (control de encendido)
GPIO 20    --->   TX (serial)
GPIO 21    <---   RX (serial)
GND        --->   GND
5V         --->   VBAT (mínimo 2A)

GPIO 9     --->   PIN_ACTIVE (control de estado)
GPIO 8     --->   PIN_INACTIVE (control de estado)
```

### Requisitos de Alimentación

- Voltaje: 3.8V - 4.2V (típicamente 5V con regulador)
- Corriente en reposo: ~20 mA
- Corriente en transmisión: hasta 2A (picos)
- Recomendado: Fuente de 5V/3A o batería Li-Ion 3.7V con capacidad >2000mAh

## Configuración

### 1. Clonar el Repositorio

```bash
git clone https://github.com/BrandonMarias/findme32.git
cd findme32
```

### 2. Configurar Credenciales

#### Para GPS Tracker (findme32):

```bash
cp src/findme32/config_template.h src/findme32/config.h
```

Editar `src/findme32/config.h`:

```cpp
#define DEVICE_TOKEN "tu_token_unico"
#define API_ENDPOINT "tu_servidor.com"
#define API_PATH "/api/gps/gpstracker"
```

#### Para Control SMS (findme):

```bash
cp src/findme/findme_config_template.h src/findme/findme_config.h
```

Editar `src/findme/findme_config.h`:

```cpp
const String numerosAutorizados[] = {
    "+521234567890",
    "+529876543210"
};
```

### 3. Configurar APN del Operador

En `config.h`, ajustar según operador:

```cpp
#define APN_TELCEL "internet.itelcel.com"
```

Operadores comunes en México:
- Telcel: `internet.itelcel.com`
- Movistar: `internet.movistar.com.mx`
- AT&T: `internet.att.com.mx`

### 4. Compilar y Cargar

```bash
platformio run --target upload
platformio device monitor
```

## Parámetros Configurables

### GPS Tracker

```cpp
UMBRAL_MOVIMIENTO_METROS    // Distancia mínima para detectar movimiento (25m)
INTERVALO_LECTURA_GPS       // Frecuencia de lectura GPS (20 segundos)
INTERVALO_HEARTBEAT         // Intervalo de envío periódico (5 minutos)
GPS_MAX_INTENTOS            // Reintentos para obtener fix GPS (20)
HTTP_TIMEOUT                // Timeout para peticiones HTTP (60s)
```

### Control SMS

```cpp
numerosAutorizados[]        // Lista de números permitidos
RELEVADOR_PIN               // Pin de control (GPIO 9)
```

## Comandos SMS Disponibles

El módulo de control SMS acepta los siguientes comandos:

- `Localizar`: Obtiene y envía la ubicación GPS actual
- `Apagar`: Activa el relevador (GPIO 9 HIGH)
- `Prender`: Desactiva el relevador (GPIO 9 LOW)

Los comandos solo son procesados si provienen de números autorizados.

## API Backend

### Endpoint de Recepción

```
GET /api/gps/gpstracker?lat={latitude}&lon={longitude}&token={device_token}&speed={speed}
```

**Parámetros:**
- `lat`: Latitud en grados decimales
- `lon`: Longitud en grados decimales
- `token`: Token único del dispositivo
- `speed`: Velocidad en km/h (opcional, solo si hay movimiento)

**Respuesta Esperada:**

```json
{
  "isActive": true
}
```

El sistema controla los pines según el valor de `isActive`:
- `true`: PIN_ACTIVE (9) encendido, PIN_INACTIVE (8) apagado
- `false`: PIN_ACTIVE (9) apagado, PIN_INACTIVE (8) encendido

## Protocolo de Comunicación

### Comandos AT Principales

**GPS:**
```
AT+CGNSSPWR=1       // Encender GPS
AT+CGNSSINFO        // Obtener coordenadas
```

**GPRS (SIM7600):**
```
AT+CGDCONT=1,"IP","internet.itelcel.com"
AT+CGACT=1,1        // Activar contexto PDP
AT+CGPADDR=1        // Verificar IP asignada
```

**HTTP/HTTPS:**
```
AT+HTTPINIT         // Inicializar HTTP
AT+HTTPPARA="CID",1
AT+HTTPSSL=1        // Habilitar SSL/TLS
AT+CSSLCFG="sslversion",0,3      // TLS 1.2
AT+CSSLCFG="authmode",0,0        // Sin validación de certificado
AT+CSSLCFG="enableSNI",0,1       // Habilitar SNI
AT+HTTPPARA="URL","https://..."
AT+HTTPACTION=0     // Ejecutar GET
AT+HTTPREAD=0,{len} // Leer respuesta
AT+HTTPTERM         // Terminar sesión
```

## Seguridad

### Archivos Protegidos

Los siguientes archivos contienen información sensible y están excluidos del repositorio:

- `src/findme32/config.h` - Credenciales de API y tokens
- `src/findme/findme_config.h` - Números de teléfono autorizados

Estos archivos están en `.gitignore` para prevenir exposición accidental.

### Recomendaciones

1. Usar tokens únicos por dispositivo
2. Implementar validación de tokens en el backend
3. Utilizar HTTPS en producción
4. Mantener actualizada la lista de números autorizados
5. Habilitar rate limiting en el servidor
6. Monitorear logs de acceso

## Resolución de Problemas

### GPS no obtiene coordenadas

- Verificar visibilidad al cielo (evitar lugares cerrados)
- El primer fix puede tardar 1-2 minutos
- Confirmar que la antena GPS esté conectada
- Aumentar `GPS_MAX_INTENTOS` si es necesario

### Sin conexión GPRS

- Verificar APN del operador
- Confirmar que hay señal celular (AT+CSQ)
- Revisar que la SIM tiene saldo/datos activos
- Comprobar que el contexto PDP esté activo (AT+CGACT?)

### Módulo GSM no responde

- Verificar conexiones TX/RX correctas
- Confirmar alimentación adecuada (mínimo 2A)
- Revisar que el baud rate sea 115200
- Intentar pulso manual en PWR_PIN

### Error SSL/TLS (715)

- Verificar sincronización de reloj
- Confirmar que SNI está habilitado
- Revisar que el servidor usa TLS 1.2
- Comprobar fecha/hora del módulo (AT+CCLK?)

## Desarrollo

### Requisitos

- PlatformIO Core 6.0+
- Python 3.7+
- Git

### Estructura del Código

El proyecto utiliza una arquitectura modular orientada a objetos:

- **Separación de responsabilidades**: Cada módulo gestiona una funcionalidad específica
- **Reutilización**: Los módulos pueden usarse en otros proyectos
- **Mantenibilidad**: Cambios localizados no afectan otros componentes
- **Testabilidad**: Cada módulo puede probarse independientemente

### Compilación

```bash
# Compilar sin cargar
pio run

# Compilar y cargar
pio run --target upload

# Monitor serial
pio device monitor --baud 115200

# Limpiar build
pio run --target clean
```

## Contribuciones

Las contribuciones son bienvenidas. Por favor:

1. Fork del repositorio
2. Crear una rama para tu feature (`git checkout -b feature/nueva-funcionalidad`)
3. Commit de cambios (`git commit -am 'Agregar nueva funcionalidad'`)
4. Push a la rama (`git push origin feature/nueva-funcionalidad`)
5. Crear Pull Request

## Licencia

Este proyecto es de código abierto bajo licencia MIT.

## Contacto

Brandon Marias - [@BrandonMarias](https://github.com/BrandonMarias)

Repositorio: [https://github.com/BrandonMarias/findme32](https://github.com/BrandonMarias/findme32)

