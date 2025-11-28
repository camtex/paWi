# paWi - WiFi BadUSB para ESP32-S3

<div>

```
               __    __  _ 
 _ __    __ _ / / /\ \ \(_)
| '_ \  / _` |\ \/  \/ /| |
| |_) || (_| | \  /\  / | |
| .__/  \__,_|  \/  \/  |_|
|_|                         
```

**Sistema de inyección de teclado HID vía WiFi para ESP32-S3**

[Características](#-características) • [Instalación](#-instalación) • [Uso](#-uso) • [Comandos](#-comandos-soportados) • [Seguridad](#-advertencias-legales)

</div>

---

## 📋 Descripción

**paWi** es un dispositivo basado en ESP32-S3 capaz de emular un teclado USB HID utilizando TinyUSB. Incluye un punto de acceso WiFi y un servidor web integrado, lo que permite enviar payloads y comandos directamente desde un navegador sin necesidad de aplicaciones externas ni conexión a Internet.

### Propósito Educativo

Este proyecto ha sido desarrollado **exclusivamente con fines educativos** para:
- Aprender sobre seguridad informática
- Comprender vulnerabilidades de dispositivos USB
- Realizar pruebas de penetración autorizadas
- Investigación en ciberseguridad

---

## ✨ Características

- 🎮 **Control WiFi Remoto**: Interfaz web moderna y responsive
- ⚡ **USB-OTG Nativo**: Usa el USB nativo del ESP32-S3 (sin adaptadores)
- ⌨️ **Teclado LATAM**: Mapeo completo optimizado para Latinoamérica
- 🚀 **Alta Performance**: Búsqueda binaria O(log n) y delays de 2-5ms
- 📦 **6 Payloads Predefinidos**: Listos para ejecutar
- ✏️ **Editor Personalizado**: Crea tus propios scripts (hasta 4KB)
- 🔒 **Rate Limiting**: Protección contra ejecuciones múltiples
- 📊 **Sistema de Logs**: Monitoreo en tiempo real

---

## 🛠️ Hardware Requerido

| Componente | Especificación |
|------------|---------------|
| **Microcontrolador** | ESP32-S3 (con USB-OTG) |
| **Cable** | USB-C 2.0+ |
| **Alimentación** | 5V / 500mA |

**Placas compatibles**: ESP32-S3-DevKitC-1, ESP32-S3-WROOM-1

> ⚠️ **NO funciona con ESP32 clásico** - requiere ESP32-S3 con USB-OTG nativo

---

## 📥 Instalación

### 1. Configurar Arduino IDE

1. Instalar [Arduino IDE 2.0+](https://www.arduino.cc/en/software)
2. Agregar URL en `Preferencias`:
```
https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json
```
3. Instalar soporte ESP32 desde el Gestor de Tarjetas

### 2. Configuración de los Tools

- **Placa**: `ESP32S3 Dev Module`
- **USB CDC On Boot**: `Enabled`
- **USB Mode**: `USB-OTG (TinyUSB)`
- **Upload Mode**: `USB-OTG CDC (TinyUSB)`


### 3. Verificación

Monitor Serie (921600 baudios) debe mostrar:
```
========================================
✓ SISTEMA LISTO - paWi 
========================================
WiFi: paWi-Link | Pass: 12345678
Web: http://192.168.4.1
========================================
```

---

## 🚀 Uso

### Conexión

1. Conectar ESP32-S3 al puerto USB objetivo
2. Conectarse a WiFi: `paWi-Link` / `12345678`
3. Abrir navegador: `http://192.168.4.1`

### Interfaz Web

- **Payloads Predefinidos**: 6 botones con acciones listas
- **Editor Personalizado**: Crear scripts Ducky Script propios
- **Panel de Estado**: IP, último payload, total de ejecuciones
- **Log del Sistema**: Monitoreo en tiempo real

### Payloads Incluidos

| # | Nombre | Descripción |
|---|--------|-------------|
| 1 | Notepad | Abre Notepad con ASCII art |
| 2 | System Info | Información del sistema |
| 3 | Clone GitHub | Clona repositorio de payloads |
| 4 | Abrir URL | Navegación automática |
| 5 | Defender OFF | Desactiva protecciones (Admin) |
| 6 | WiFi Pass | Exporta contraseñas WiFi |

> ⏱️ **Importante**: Espera 5 segundos entre ejecuciones

---

## 📝 Comandos Soportados

### Sintaxis Ducky Script
```ducky
GUI r
DELAY 500
STRING notepad
ENTER
DELAY 1000
STRING Hola desde paWi!
```

### Tabla de Comandos

| Comando | Descripción | Ejemplo |
|---------|-------------|---------|
| `STRING` | Escribe texto | `STRING Hola Mundo` |
| `ENTER` | Presiona Enter | `ENTER` |
| `DELAY` | Pausa en ms | `DELAY 1000` |
| `GUI` | Tecla Windows | `GUI r` |
| `ALT` | Tecla Alt | `ALT F4` |
| `CTRL` | Tecla Control | `CTRL c` |
| `SHIFT` | Tecla Shift | `SHIFT DELETE` |
| `TAB` | Tabulador | `TAB` |
| `ESC` | Escape | `ESC` |
| `SPACE` | Espacio | `SPACE` |
| `F1-F12` | Teclas función | `F5` |
| `UP/DOWN/LEFT/RIGHT` | Flechas | `DOWN` |
| `HOME/END` | Navegación | `HOME` |
| `PAGEUP/PAGEDOWN` | Paginación | `PAGEUP` |
| `INSERT/DELETE` | Edición | `DELETE` |
| `BACKSPACE` | Retroceso | `BACKSPACE` |

### Escapes Soportados

- `\n` → Nueva línea
- `\t` → Tabulador
- `\r` → Retorno de carro
- `\\` → Barra invertida literal
- `\"` → Comillas dobles

---

## 🔧 Personalización

### Cambiar Credenciales WiFi
```cpp
const char* ap_ssid = "TuNombre";
const char* ap_password = "TuPassword";
```

### Ajustar Timeouts
```cpp
#define DELAY_INITIAL 5000    // Espera inicial USB (ms)
#define DELAY_SHORT 100       // Delays cortos (ms)
#define DELAY_MEDIUM 500      // Delays medios (ms)
#define PAYLOAD_TIMEOUT 30000 // Timeout máximo (ms)
```

---

## 🔒 ADVERTENCIAS LEGALES

### ⚠️ USO EXCLUSIVAMENTE EDUCATIVO

Este proyecto es para:
- ✅ Investigación de seguridad autorizada
- ✅ Pruebas en sistemas propios
- ✅ Educación en ciberseguridad
- ✅ Pentesting con consentimiento explícito

### ❌ PROHIBIDO USAR PARA:
- Acceso no autorizado a sistemas
- Robo de información
- Daño a equipos o datos
- Cualquier actividad ilegal

> **El uso indebido puede resultar en consecuencias legales graves. El autor NO se responsabiliza del mal uso de esta herramienta.**

### Responsabilidad del Usuario

Al usar este software, aceptas:
1. Obtener autorización por escrito antes de cualquier prueba
2. Usar únicamente en entornos controlados
3. Cumplir con todas las leyes locales e internacionales
4. Asumir total responsabilidad de tus acciones

---


## 📄 Licencia

Este proyecto está bajo la Licencia MIT. Ver [LICENSE](LICENSE) para más detalles.
```
MIT License - Copyright (c) 2025 paWi 
```

---

## 💌 Autor

**Camila Warthon**
- GitHub: [@camtex](https://github.com/camtex)
- Linkedin: [Camila Warthon](https://www.linkedin.com/in/camila-warthon-33744634b/)

---
