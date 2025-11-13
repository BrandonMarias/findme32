# ✅ Configuración Actualizada - GPS Tracker

## 🎯 Cambios Realizados

### 1. **URL del Backend Actualizada**

**Antes:**
```cpp
#define API_ENDPOINT "mibackend.com"
#define API_PATH "/gpstracker"
```

**Ahora:**
```cpp
#define API_ENDPOINT "tunel.ponlecoco.com"
#define API_PATH "/api/gps/gpstracker"
```

### 2. **Token del Dispositivo Configurado**

**Token único:**
```cpp
#define DEVICE_TOKEN "dt_80ced305b98eaf5d73b39281db9a39529ad6523d46a839e3873c2d69fc896f43"
```

### 3. **HTTPS Habilitado (Puerto 443)**

**Antes:**
```cpp
#define API_PORT "80"  // HTTP
String url = "http://..." 
```

**Ahora:**
```cpp
#define API_PORT "443"  // HTTPS
String url = "https://..."
AT+HTTPSSL=1  // SSL/TLS habilitado
```

### 4. **APN de Telcel Configurado**

```cpp
AT+CGDCONT=1,"IP","internet.itelcel.com"
```

---

## 📡 URL Completa de Envío

```
https://tunel.ponlecoco.com/api/gps/gpstracker?lat=19.432608&lon=-99.133209&token=dt_80ced305b98eaf5d73b39281db9a39529ad6523d46a839e3873c2d69fc896f43
```

**Parámetros:**
- `lat`: Latitud en grados decimales
- `lon`: Longitud en grados decimales  
- `token`: Token único del dispositivo

---

## 🔐 Configuración de Seguridad

✅ **HTTPS/SSL**: Habilitado con `AT+HTTPSSL=1`
✅ **Puerto seguro**: 443
✅ **Token largo**: 64 caracteres hexadecimales
✅ **Operador**: Telcel con APN `internet.itelcel.com`

---

## 📝 Comandos AT HTTPS Utilizados

```
AT+HTTPINIT               // Inicializar HTTP
AT+HTTPPARA="CID",1       // Configurar contexto
AT+HTTPSSL=1              // ⭐ Habilitar SSL/TLS
AT+HTTPPARA="URL","https://..."  // URL completa
AT+HTTPACTION=0           // GET request
AT+HTTPREAD               // Leer respuesta
AT+HTTPTERM               // Terminar sesión
```

---

## 🧪 Cómo Probar

### 1. Compilar y cargar el código:
```bash
pio run --target upload
pio device monitor
```

### 2. Verificar en el Serial Monitor:

```
>> GPS Tracker - FindMe32
>> Device Token: dt_80ced305b98eaf5d73b39281db9a39529ad6523d46a839e3873c2d69fc896f43
>> API Endpoint: tunel.ponlecoco.com/api/gps/gpstracker
>> Intervalo: 5 minutos

>> Configurando APN Telcel...
>> Activando contexto PDP...
>> ✓ GPRS conectado y listo

>> Inicializando HTTPS...
>> Habilitando SSL/TLS...
>> URL: https://tunel.ponlecoco.com/api/gps/gpstracker?lat=...&lon=...&token=dt_...
+HTTPACTION: 0,200,47
>> ✓ Ubicación enviada exitosamente (200 OK)
```

### 3. Verificar en el backend:

Tu backend en `tunel.ponlecoco.com` debe recibir:

```json
GET /api/gps/gpstracker?lat=19.432608&lon=-99.133209&token=dt_80ced305b98eaf5d73b39281db9a39529ad6523d46a839e3873c2d69fc896f43
```

---

## 🔧 Backend NestJS - Actualización

Tu controlador debe aceptar el token en query string:

```typescript
@Get()
async receiveGpsDataGet(
  @Query('lat') latitude: string,
  @Query('lon') longitude: string,
  @Query('token') deviceToken: string,
) {
  // Validar token
  if (deviceToken !== 'dt_80ced305b98eaf5d73b39281db9a39529ad6523d46a839e3873c2d69fc896f43') {
    throw new UnauthorizedException('Invalid device token');
  }
  
  // Procesar ubicación...
  return {
    success: true,
    message: 'Location received successfully',
    data: { /* ... */ }
  };
}
```

---

## ✅ Lista de Verificación

- [x] URL actualizada a `tunel.ponlecoco.com`
- [x] Ruta correcta `/api/gps/gpstracker`
- [x] Token configurado (64 caracteres)
- [x] HTTPS habilitado (`AT+HTTPSSL=1`)
- [x] Puerto 443 configurado
- [x] APN Telcel configurado
- [x] Token en query string
- [x] Documentación actualizada

---

## 🚀 El código está listo para desplegar

Todos los ajustes han sido aplicados según tus especificaciones.
