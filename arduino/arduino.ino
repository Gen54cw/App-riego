#include <WiFi.h>
#include <WebServer.h>
#include <DHT.h>
#include <ThingSpeak.h>
#include <ArduinoJson.h>

// ============================================
// CONFIGURACIÓN WiFi y ThingSpeak
// ============================================
const char* ssid = "PRUEBA";
const char* password = "gaticu54";

// ThingSpeak
unsigned long channelID = 3170485;  // Tu Channel ID
const char* writeAPIKey = "UXMGLNI3EFQMC7GT";

WiFiClient client;
WebServer server(80);

// ============================================
// CONFIGURACIÓN HARDWARE - NodeMCU ESP32
// ============================================
#define DHTPIN 4          // GPIO4 (equivalente a D4)
#define DHTTYPE DHT22
#define SENSOR_SUELO 34   // GPIO34 (ADC1_CH6) - Pin analógico
#define RELE 5            // GPIO5 (equivalente a D1)
#define PIN_FLUJO 2       // GPIO2 (equivalente a D2)

DHT dht(DHTPIN, DHTTYPE);

// --- Variables Sensor Flujo ---
volatile int pulsoContador = 0;
float factorK = 7.5;
float flujoLitrosPorMinuto = 0;
unsigned long tiempoAntiguo = 0;

// ============================================
// TIPOS DE CULTIVOS
// ============================================
struct ConfigCultivo {
  String nombre;
  int humedadMinima;
  int humedadOptima;
  int humedadMaxima;
  float factorAgua;
  int frecuenciaRiego;
  String descripcion;
};

ConfigCultivo cultivos[] = {
  {"Tomate", 60, 70, 85, 1.2, 60, "Requiere riego constante, alta humedad"},
  {"Lechuga", 65, 75, 90, 1.4, 45, "Necesita mucha agua, raices superficiales"},
  {"Zanahoria", 50, 65, 80, 1.0, 90, "Riego moderado, evitar encharcamiento"},
  {"Pimiento", 55, 70, 85, 1.1, 60, "Riego regular, sensible a sequia"},
  {"Fresa", 60, 75, 90, 1.3, 50, "Alta demanda de agua, raices superficiales"},
  {"Maiz", 45, 60, 75, 0.9, 120, "Riego profundo pero espaciado"},
  {"Pepino", 70, 80, 95, 1.5, 40, "Muy alta necesidad de agua"},
  {"Cebolla", 40, 55, 70, 0.8, 100, "Riego moderado, tolerante a sequia"},
  {"Albahaca", 55, 70, 85, 1.1, 60, "Mantener humedad constante"},
  {"Espinaca", 60, 75, 90, 1.3, 50, "Alta humedad, crece rapido"}
};

int numCultivos = 10;
int cultivoSeleccionado = 0;

// ============================================
// ESTRUCTURAS DEL SISTEMA
// ============================================
struct ConfigAutonoma {
  int umbralHumedadMin = 60;
  int umbralHumedadMax = 85;
  int umbralHumedadOptimo = 70;
  int sueloSecoMax = 4095;  // ESP32 usa ADC de 12 bits (0-4095)
  int sueloHumedoMin = 0;
  bool calibracionCompletada = false;
  unsigned long tiempoRiegoBase = 30000;
  unsigned long tiempoRiegoActual = 30000;
  enum TipoSuelo { ARENOSO, ARCILLOSO, LIMOSO, DESCONOCIDO };
  TipoSuelo tipoSuelo = DESCONOCIDO;
  float factorTemperatura = 1.0;
  float factorCultivo = 1.0;
} config;

struct SistemaAutoReparacion {
  int fallosDHT = 0;
  bool dhtOperativo = true;
  float ultimaTempValida = 25.0;
  float ultimaHumAireValida = 50.0;
  int ultimaHumSueloValida = 50;
  int intentosRecuperacionDHT = 0;
  unsigned long ultimoIntentoRecuperacion = 0;
  bool modoSeguro = false;
} autoReparacion;

struct SistemaOptimizacion {
  int ciclosRiego = 0;
  float aguaTotalUsada = 0;
  unsigned long tiempoRiegoTotal = 0;
  float eficienciaPromedio = 0;
  float mejorEficiencia = 0;
  int humedadAntesRiego[10] = {0};
  int humedadDespuesRiego[10] = {0};
  int indiceHistorial = 0;
  unsigned long tiempoOptimo = 30000;
  float tasaAbsorcion = 0;
  int ciclosSinAjuste = 0;
} optimizacion;

struct SistemaProteccion {
  float tempMaxSegura = 45.0;
  float tempMinSegura = 0.0;
  float flujoMaxSeguro = 15.0;
  unsigned long tiempoMaxRiegoContinuo = 300000;
  bool sobreCalentamiento = false;
  bool fugatDetectada = false;
  bool bloqueoBomba = false;
  unsigned long tiempoRiegoActual = 0;
  unsigned long inicioRiego = 0;
  int riegosConsecutivos = 0;
  unsigned long ultimoRiego = 0;
  unsigned long tiempoMinimoEntreRiegos = 60000;
  bool emergenciaActiva = false;
  String razonEmergencia = "";
  unsigned long tiempoInicioEmergencia = 0;
  int nivelEmergencia = 0;  // 0=normal, 1=advertencia, 2=crítica
} proteccion;

enum EstadoSistema { INICIANDO, CALIBRANDO, NORMAL, RIEGO_ACTIVO, MODO_SEGURO, EMERGENCIA };
EstadoSistema estadoActual = INICIANDO;
unsigned long tiempoInicioEstado = 0;

float temperaturaActual = 0;
float humedadAireActual = 0;
int humedadSueloActual = 0;

unsigned long ultimoEnvioThingSpeak = 0;
const unsigned long intervaloThingSpeak = 20000;

// ============================================
// FUNCIONES AUXILIARES
// ============================================
void IRAM_ATTR contarPulsos() {
  pulsoContador++;
}

String getTipoSuelo() {
  switch(config.tipoSuelo) {
    case ConfigAutonoma::ARENOSO: return "Arenoso";
    case ConfigAutonoma::ARCILLOSO: return "Arcilloso";
    case ConfigAutonoma::LIMOSO: return "Limoso";
    default: return "Detectando";
  }
}

String getEstadoSistema() {
  switch(estadoActual) {
    case INICIANDO: return "Iniciando";
    case CALIBRANDO: return "Calibrando";
    case NORMAL: return "Normal";
    case RIEGO_ACTIVO: return "Riego Activo";
    case MODO_SEGURO: return "Modo Seguro";
    case EMERGENCIA: return "Emergencia";
    default: return "Desconocido";
  }
}

void aplicarConfiguracionCultivo() {
  ConfigCultivo cultivo = cultivos[cultivoSeleccionado];
  config.umbralHumedadMin = cultivo.humedadMinima;
  config.umbralHumedadOptimo = cultivo.humedadOptima;
  config.umbralHumedadMax = cultivo.humedadMaxima;
  config.factorCultivo = cultivo.factorAgua;
  proteccion.tiempoMinimoEntreRiegos = cultivo.frecuenciaRiego * 60000;
  
  Serial.print("Cultivo seleccionado: ");
  Serial.println(cultivo.nombre);
}

// ============================================
// PRINCIPIO 1: AUTO-CONFIGURACIÓN
// ============================================
void autoCalibracion() {
  static unsigned long inicioCalibracion = 0;
  static int lecturasCalibracion = 0;
  static int lecturasAnteriores[10] = {0};
  static int indiceLectura = 0;
  
  if (inicioCalibracion == 0) {
    inicioCalibracion = millis();
    config.sueloSecoMax = 0;
    config.sueloHumedoMin = 4095;
    Serial.println("🔧 [AUTO-CONFIG] Iniciando calibración del suelo...");
  }
  
  int lecturaActual = analogRead(SENSOR_SUELO);
  
  // Validar lectura
  if (lecturaActual < 0 || lecturaActual > 4095) {
    return;
  }
  
  // Actualizar rangos
  if (lecturaActual > config.sueloSecoMax) config.sueloSecoMax = lecturaActual;
  if (lecturaActual < config.sueloHumedoMin) config.sueloHumedoMin = lecturaActual;
  
  // Acumular lecturas
  lecturasAnteriores[indiceLectura] = lecturaActual;
  indiceLectura = (indiceLectura + 1) % 10;
  lecturasCalibracion++;
  
  // Mínimo 30 segundos de calibración
  unsigned long tiempoCalibracion = millis() - inicioCalibracion;
  if (tiempoCalibracion < 30000) {
    if (lecturasCalibracion % 10 == 0) {
      Serial.print("🔧 [AUTO-CONFIG] Calibrando... ");
      Serial.print((tiempoCalibracion / 1000));
      Serial.println("s");
    }
    return;
  }
  
  // Analizar variabilidad para detectar tipo de suelo
  int variacionMax = 0;
  int variacionMin = 4095;
  for (int i = 0; i < 10; i++) {
    if (lecturasAnteriores[i] > variacionMax) variacionMax = lecturasAnteriores[i];
    if (lecturasAnteriores[i] < variacionMin) variacionMin = lecturasAnteriores[i];
  }
  int variacion = variacionMax - variacionMin;
  
  // Clasificar tipo de suelo
  if (variacion < 200) {
    config.tipoSuelo = ConfigAutonoma::ARCILLOSO;
    config.tiempoRiegoBase = 45000;
    config.tiempoRiegoActual = 45000;
  } else if (variacion > 600) {
    config.tipoSuelo = ConfigAutonoma::ARENOSO;
    config.tiempoRiegoBase = 20000;
    config.tiempoRiegoActual = 20000;
  } else {
    config.tipoSuelo = ConfigAutonoma::LIMOSO;
    config.tiempoRiegoBase = 30000;
    config.tiempoRiegoActual = 30000;
  }
  
  config.calibracionCompletada = true;
  Serial.print("✅ [AUTO-CONFIG] Calibración completada - Suelo: ");
  Serial.print(getTipoSuelo());
  Serial.print(", Rango: ");
  Serial.print(config.sueloHumedoMin);
  Serial.print("-");
  Serial.print(config.sueloSecoMax);
  Serial.print(", Tiempo base: ");
  Serial.print(config.tiempoRiegoBase / 1000);
  Serial.println("s");
  
  inicioCalibracion = 0; // Reset para próxima calibración
}

bool diagnosticarDHT(float temp, float hum) {
  if (isnan(temp) || isnan(hum)) {
    autoReparacion.fallosDHT++;
    if (autoReparacion.fallosDHT >= 3) {
      autoReparacion.dhtOperativo = false;
      return false;
    }
  } else {
    autoReparacion.fallosDHT = 0;
    autoReparacion.dhtOperativo = true;
    autoReparacion.ultimaTempValida = temp;
    autoReparacion.ultimaHumAireValida = hum;
  }
  return true;
}

void intentarRecuperarDHT() {
  if (millis() - autoReparacion.ultimoIntentoRecuperacion > 30000) {
    dht.begin();
    autoReparacion.intentosRecuperacionDHT++;
    autoReparacion.ultimoIntentoRecuperacion = millis();
    if (autoReparacion.intentosRecuperacionDHT > 5) {
      autoReparacion.modoSeguro = true;
    }
  }
}

bool diagnosticarFlujo() {
  if (digitalRead(RELE) == HIGH && flujoLitrosPorMinuto < 0.1 && 
      millis() - proteccion.inicioRiego > 5000) {
    proteccion.bloqueoBomba = true;
    return false;
  }
  if (digitalRead(RELE) == LOW && flujoLitrosPorMinuto > 0.5) {
    proteccion.fugatDetectada = true;
    return false;
  }
  return true;
}

void optimizarParametros() {
  optimizacion.ciclosSinAjuste++;
  if (optimizacion.ciclosRiego > 0 && optimizacion.ciclosRiego % 10 == 0) {
    float eficienciaActual = 0;
    if (optimizacion.aguaTotalUsada > 0) {
      float humedadGanada = 0;
      for (int i = 0; i < 10; i++) {
        humedadGanada += (optimizacion.humedadDespuesRiego[i] - optimizacion.humedadAntesRiego[i]);
      }
      eficienciaActual = humedadGanada / optimizacion.aguaTotalUsada;
    }
    if (eficienciaActual > optimizacion.mejorEficiencia) {
      optimizacion.mejorEficiencia = eficienciaActual;
      optimizacion.tiempoOptimo = config.tiempoRiegoActual;
    } else {
      if (optimizacion.ciclosSinAjuste > 5) {
        config.tiempoRiegoActual = (config.tiempoRiegoActual + optimizacion.tiempoOptimo) / 2;
        optimizacion.ciclosSinAjuste = 0;
      }
    }
  }
}

void registrarCicloRiego(int humedadAntes, int humedadDespues, float aguaUsada) {
  int idx = optimizacion.indiceHistorial % 10;
  optimizacion.humedadAntesRiego[idx] = humedadAntes;
  optimizacion.humedadDespuesRiego[idx] = humedadDespues;
  optimizacion.indiceHistorial++;
  optimizacion.ciclosRiego++;
  optimizacion.aguaTotalUsada += aguaUsada;
  if (config.tiempoRiegoActual > 0) {
    optimizacion.tasaAbsorcion = (humedadDespues - humedadAntes) / (config.tiempoRiegoActual / 1000.0);
  }
  if (optimizacion.aguaTotalUsada > 0) {
    optimizacion.eficienciaPromedio = ((humedadDespues - humedadAntes) / optimizacion.aguaTotalUsada) * 100;
  }
}

bool verificarCondicionesSeguras(float temp, int humedadSuelo) {
  bool seguro = true;
  if (temp > proteccion.tempMaxSegura) {
    proteccion.sobreCalentamiento = true;
    proteccion.emergenciaActiva = true;
    proteccion.razonEmergencia = "Temperatura excesiva";
    seguro = false;
  }
  if (temp < proteccion.tempMinSegura) {
    proteccion.emergenciaActiva = true;
    proteccion.razonEmergencia = "Riesgo de congelacion";
    seguro = false;
  }
  if (digitalRead(RELE) == HIGH) {
    if (millis() - proteccion.inicioRiego > proteccion.tiempoMaxRiegoContinuo) {
      proteccion.emergenciaActiva = true;
      proteccion.razonEmergencia = "Tiempo maximo de riego excedido";
      seguro = false;
    }
  }
  if (humedadSuelo > config.umbralHumedadMax) {
    proteccion.emergenciaActiva = true;
    proteccion.razonEmergencia = "Suelo sobresaturado";
    seguro = false;
  }
  if (millis() - proteccion.ultimoRiego < proteccion.tiempoMinimoEntreRiegos) {
    seguro = false;
  }
  return seguro;
}

void manejarEmergencia() {
  digitalWrite(RELE, LOW);
  estadoActual = EMERGENCIA;
  
  if (tiempoInicioEstado == 0) {
    tiempoInicioEstado = millis();
    Serial.println("🚨 [AUTO-PROT] EMERGENCIA: " + proteccion.razonEmergencia);
  }
  
  // Auto-recuperación después de 60 segundos
  if (millis() - tiempoInicioEstado > 60000) {
    proteccion.emergenciaActiva = false;
    proteccion.sobreCalentamiento = false;
    proteccion.fugatDetectada = false;
    proteccion.bloqueoBomba = false;
    proteccion.nivelEmergencia = 0;
    proteccion.razonEmergencia = "";
    estadoActual = NORMAL;
    tiempoInicioEstado = millis();
    Serial.println("✅ [AUTO-PROT] Emergencia resuelta, sistema normalizado");
  } else {
    unsigned long tiempoRestante = (60000 - (millis() - tiempoInicioEstado)) / 1000;
    if (tiempoRestante % 10 == 0 && tiempoRestante > 0) {
      Serial.print("⏳ [AUTO-PROT] Recuperación automática en ");
      Serial.print(tiempoRestante);
      Serial.println(" segundos");
    }
  }
}

void controlarRiego(int humedadSuelo, float temperatura) {
  bool debeRegar = false;
  if (temperatura > 30) {
    config.factorTemperatura = 1.3;
  } else if (temperatura < 15) {
    config.factorTemperatura = 0.7;
  } else {
    config.factorTemperatura = 1.0;
  }
  
  int umbralAjustado = config.umbralHumedadMin * config.factorTemperatura * config.factorCultivo;
  umbralAjustado = constrain(umbralAjustado, 0, 100);
  
  if (humedadSuelo < umbralAjustado) {
    debeRegar = true;
  }
  
  if (debeRegar && verificarCondicionesSeguras(temperatura, humedadSuelo)) {
    if (digitalRead(RELE) == LOW) {
      digitalWrite(RELE, HIGH);
      proteccion.inicioRiego = millis();
      estadoActual = RIEGO_ACTIVO;
      proteccion.riegosConsecutivos++;
    }
  } else {
    if (digitalRead(RELE) == HIGH) {
      digitalWrite(RELE, LOW);
      unsigned long duracionRiego = millis() - proteccion.inicioRiego;
      float aguaUsada = (flujoLitrosPorMinuto * duracionRiego) / 60000.0;
      proteccion.ultimoRiego = millis();
      optimizacion.tiempoRiegoTotal += duracionRiego;
      registrarCicloRiego(humedadSuelo - 10, humedadSuelo, aguaUsada);
      estadoActual = NORMAL;
      proteccion.riegosConsecutivos = 0;
    }
  }
}

void enviarDatosThingSpeak() {
  if (millis() - ultimoEnvioThingSpeak >= intervaloThingSpeak) {
    ThingSpeak.setField(1, temperaturaActual);
    ThingSpeak.setField(2, humedadAireActual);
    ThingSpeak.setField(3, humedadSueloActual);
    ThingSpeak.setField(4, flujoLitrosPorMinuto);
    ThingSpeak.setField(5, optimizacion.aguaTotalUsada);
    ThingSpeak.setField(6, digitalRead(RELE) == HIGH ? 1 : 0);
    ThingSpeak.setField(7, optimizacion.ciclosRiego);
    ThingSpeak.setField(8, optimizacion.eficienciaPromedio);
    
    int statusCode = ThingSpeak.writeFields(channelID, writeAPIKey);
    if (statusCode == 200) {
      Serial.println("ThingSpeak OK");
    }
    ultimoEnvioThingSpeak = millis();
  }
}

// ============================================
// API REST - ENDPOINTS SIMPLIFICADOS
// ============================================

void setCORSHeaders() {
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.sendHeader("Access-Control-Allow-Methods", "GET, POST, OPTIONS");
  server.sendHeader("Access-Control-Allow-Headers", "Content-Type");
}

void handleOptions() {
  setCORSHeaders();
  server.send(204);
}

void handleGetData() {
  setCORSHeaders();
  
  StaticJsonDocument<1024> doc;
  
  // Datos de sensores
  doc["temperatura"] = temperaturaActual;
  doc["humedadAire"] = humedadAireActual;
  doc["humedadSuelo"] = humedadSueloActual;
  doc["flujo"] = flujoLitrosPorMinuto;
  
  // Estado del sistema
  doc["estado"] = getEstadoSistema();
  doc["bombaActiva"] = (digitalRead(RELE) == HIGH);
  
  // Estadísticas
  doc["ciclosRiego"] = optimizacion.ciclosRiego;
  doc["aguaTotal"] = optimizacion.aguaTotalUsada;
  doc["eficiencia"] = optimizacion.eficienciaPromedio;
  doc["tasaAbsorcion"] = optimizacion.tasaAbsorcion;
  
  // Configuración
  doc["tipoSuelo"] = getTipoSuelo();
  doc["calibracionCompletada"] = config.calibracionCompletada;
  doc["cultivoId"] = cultivoSeleccionado;
  doc["cultivoNombre"] = cultivos[cultivoSeleccionado].nombre;
  doc["umbralHumedadMin"] = config.umbralHumedadMin;
  doc["umbralHumedadOptimo"] = config.umbralHumedadOptimo;
  doc["umbralHumedadMax"] = config.umbralHumedadMax;
  doc["tiempoRiegoActual"] = config.tiempoRiegoActual / 1000; // en segundos
  
  // Auto-reparación
  doc["dhtOperativo"] = autoReparacion.dhtOperativo;
  doc["modoSeguro"] = autoReparacion.modoSeguro;
  
  // Auto-protección
  doc["bloqueoBomba"] = proteccion.bloqueoBomba;
  doc["fuga"] = proteccion.fugatDetectada;
  doc["emergenciaActiva"] = proteccion.emergenciaActiva;
  doc["razonEmergencia"] = proteccion.razonEmergencia;
  doc["nivelEmergencia"] = proteccion.nivelEmergencia;
  doc["sobreCalentamiento"] = proteccion.sobreCalentamiento;
  
  // Tiempos
  unsigned long tiempoRiegoActual = 0;
  if (digitalRead(RELE) == HIGH) {
    tiempoRiegoActual = (millis() - proteccion.inicioRiego) / 1000;
  }
  doc["tiempoRiegoTranscurrido"] = tiempoRiegoActual;
  doc["tiempoMaxRiego"] = proteccion.tiempoMaxRiegoContinuo / 1000;
  doc["tiempoMinimoEntreRiegos"] = proteccion.tiempoMinimoEntreRiegos / 1000;
  
  // Auto-optimización
  doc["tiempoOptimo"] = optimizacion.tiempoOptimo / 1000;
  doc["mejorEficiencia"] = optimizacion.mejorEficiencia;
  
  // ThingSpeak
  doc["channelID"] = channelID;
  doc["wifiConectado"] = (WiFi.status() == WL_CONNECTED);
  doc["ip"] = WiFi.localIP().toString();
  
  // Timestamp
  doc["timestamp"] = millis();
  
  String response;
  serializeJson(doc, response);
  server.send(200, "application/json", response);
}

void handleGetCultivos() {
  setCORSHeaders();
  
  DynamicJsonDocument doc(2048);
  JsonArray array = doc.to<JsonArray>();
  
  for (int i = 0; i < numCultivos; i++) {
    JsonObject cultivo = array.createNestedObject();
    cultivo["id"] = i;
    cultivo["nombre"] = cultivos[i].nombre;
    cultivo["humedadMinima"] = cultivos[i].humedadMinima;
    cultivo["humedadOptima"] = cultivos[i].humedadOptima;
    cultivo["humedadMaxima"] = cultivos[i].humedadMaxima;
    cultivo["factorAgua"] = cultivos[i].factorAgua;
    cultivo["frecuenciaRiego"] = cultivos[i].frecuenciaRiego;
    cultivo["descripcion"] = cultivos[i].descripcion;
  }
  
  String response;
  serializeJson(doc, response);
  server.send(200, "application/json", response);
}

void handleSetCultivo() {
  setCORSHeaders();
  
  if (server.hasArg("id")) {
    int id = server.arg("id").toInt();
    if (id >= 0 && id < numCultivos) {
      cultivoSeleccionado = id;
      aplicarConfiguracionCultivo();
      
      StaticJsonDocument<128> doc;
      doc["status"] = "ok";
      doc["message"] = "Cultivo actualizado";
      doc["cultivoId"] = id;
      doc["cultivoNombre"] = cultivos[id].nombre;
      
      String response;
      serializeJson(doc, response);
      server.send(200, "application/json", response);
    } else {
      server.send(400, "application/json", "{\"status\":\"error\",\"message\":\"ID invalido\"}");
    }
  } else {
    server.send(400, "application/json", "{\"status\":\"error\",\"message\":\"Falta parametro id\"}");
  }
}

void handleSetConfig() {
  setCORSHeaders();
  
  bool configActualizada = false;
  String detalles = "";
  
  // Umbrales de humedad
  if (server.hasArg("umbralMin")) {
    int valor = server.arg("umbralMin").toInt();
    if (valor >= 0 && valor <= 100) {
      config.umbralHumedadMin = valor;
      configActualizada = true;
      detalles += "Humedad mínima: " + String(valor) + "%. ";
    }
  }
  
  if (server.hasArg("umbralOptimo")) {
    int valor = server.arg("umbralOptimo").toInt();
    if (valor >= 0 && valor <= 100) {
      config.umbralHumedadOptimo = valor;
      configActualizada = true;
      detalles += "Humedad óptima: " + String(valor) + "%. ";
    }
  }
  
  if (server.hasArg("umbralMax")) {
    int valor = server.arg("umbralMax").toInt();
    if (valor >= 0 && valor <= 100) {
      config.umbralHumedadMax = valor;
      configActualizada = true;
      detalles += "Humedad máxima: " + String(valor) + "%. ";
    }
  }
  
  // Tiempos de riego
  if (server.hasArg("tiempoRiego")) {
    int valor = server.arg("tiempoRiego").toInt();
    if (valor >= 10 && valor <= 600) {
      config.tiempoRiegoActual = valor * 1000; // Convertir a milisegundos
      config.tiempoRiegoBase = valor * 1000;
      configActualizada = true;
      detalles += "Tiempo de riego: " + String(valor) + "s. ";
    }
  }
  
  if (server.hasArg("tiempoMaxRiego")) {
    int valor = server.arg("tiempoMaxRiego").toInt();
    if (valor >= 60 && valor <= 1800) {
      proteccion.tiempoMaxRiegoContinuo = valor * 1000; // Convertir a milisegundos
      configActualizada = true;
      detalles += "Tiempo máximo de riego: " + String(valor) + "s. ";
    }
  }
  
  if (server.hasArg("tiempoMinEntreRiegos")) {
    int valor = server.arg("tiempoMinEntreRiegos").toInt();
    if (valor >= 30 && valor <= 600) {
      proteccion.tiempoMinimoEntreRiegos = valor * 1000; // Convertir a milisegundos
      configActualizada = true;
      detalles += "Tiempo mínimo entre riegos: " + String(valor) + "s. ";
    }
  }
  
  // Temperaturas
  if (server.hasArg("tempMax")) {
    float valor = server.arg("tempMax").toFloat();
    if (valor >= 0 && valor <= 100) {
      proteccion.tempMaxSegura = valor;
      configActualizada = true;
      detalles += "Temperatura máxima: " + String(valor, 1) + "°C. ";
    }
  }
  
  if (server.hasArg("tempMin")) {
    float valor = server.arg("tempMin").toFloat();
    if (valor >= -20 && valor <= 50) {
      proteccion.tempMinSegura = valor;
      configActualizada = true;
      detalles += "Temperatura mínima: " + String(valor, 1) + "°C. ";
    }
  }
  
  if (configActualizada) {
    StaticJsonDocument<256> doc;
    doc["status"] = "ok";
    doc["message"] = "Configuración actualizada";
    doc["detalles"] = detalles;
    
    Serial.println("⚙️ [CONFIG] Configuración manual actualizada: " + detalles);
    
    String response;
    serializeJson(doc, response);
    server.send(200, "application/json", response);
  } else {
    server.send(400, "application/json", "{\"status\":\"error\",\"message\":\"No se recibieron parámetros válidos\"}");
  }
}

void handleCommand() {
  setCORSHeaders();
  
  if (server.hasArg("cmd")) {
    String cmd = server.arg("cmd");
    String message = "";
    bool success = true;
    String detalles = "";
    
    if (cmd == "calibrate") {
      // AUTO-CONFIGURACIÓN: Iniciar calibración completa
      estadoActual = CALIBRANDO;
      config.calibracionCompletada = false;
      config.sueloSecoMax = 0;
      config.sueloHumedoMin = 4095;
      tiempoInicioEstado = millis();
      message = "Auto-configuración iniciada";
      detalles = "El sistema está analizando el tipo de suelo. Esto tomará aproximadamente 30 segundos.";
      Serial.println("🔧 [AUTO-CONFIG] Calibración iniciada manualmente");
    } 
    else if (cmd == "optimize") {
      // AUTO-OPTIMIZACIÓN: Forzar optimización
      if (optimizacion.ciclosRiego > 0) {
        optimizarParametros();
        message = "Auto-optimización completada";
        detalles = "Tiempo óptimo ajustado a " + String(optimizacion.tiempoOptimo / 1000) + " segundos. Eficiencia: " + String(optimizacion.eficienciaPromedio, 1) + "%";
        Serial.println("⚡ [AUTO-OPT] Optimización forzada - Tiempo óptimo: " + String(optimizacion.tiempoOptimo / 1000) + "s");
      } else {
        message = "No hay suficientes datos";
        detalles = "Se necesitan al menos 1 ciclo de riego para optimizar";
        success = false;
      }
    } 
    else if (cmd == "emergency") {
      // AUTO-PROTECCIÓN: Activar emergencia manual
      proteccion.emergenciaActiva = true;
      proteccion.razonEmergencia = "Activación manual por usuario";
      proteccion.tiempoInicioEmergencia = millis();
      proteccion.nivelEmergencia = 2;
      digitalWrite(RELE, LOW);
      estadoActual = EMERGENCIA;
      tiempoInicioEstado = millis();
      message = "Modo emergencia activado";
      detalles = "Sistema detenido por seguridad. Se reactivará automáticamente en 60 segundos.";
      Serial.println("🚨 [AUTO-PROT] EMERGENCIA ACTIVADA MANUALMENTE");
    } 
    else if (cmd == "reset") {
      // Resetear estadísticas
      optimizacion.ciclosRiego = 0;
      optimizacion.aguaTotalUsada = 0;
      optimizacion.eficienciaPromedio = 0;
      optimizacion.mejorEficiencia = 0;
      optimizacion.indiceHistorial = 0;
      optimizacion.ciclosSinAjuste = 0;
      optimizacion.tiempoRiegoTotal = 0;
      optimizacion.tasaAbsorcion = 0;
      optimizacion.tiempoOptimo = 30000;
      for (int i = 0; i < 10; i++) {
        optimizacion.humedadAntesRiego[i] = 0;
        optimizacion.humedadDespuesRiego[i] = 0;
      }
      
      // Resetear valores de configuración a sus valores iniciales POR DEFECTO
      config.umbralHumedadMin = 60;
      config.umbralHumedadOptimo = 70;
      config.umbralHumedadMax = 85;
      config.tiempoRiegoBase = 30000;
      config.tiempoRiegoActual = 30000;
      config.factorTemperatura = 1.0;
      config.factorCultivo = 1.0;
      
      // Resetear valores de protección a sus valores iniciales
      proteccion.tiempoMaxRiegoContinuo = 300000;  // 5 minutos
      proteccion.tempMaxSegura = 45.0;
      proteccion.tempMinSegura = 0.0;
      
      // Resetear estado del sistema
      proteccion.emergenciaActiva = false;
      proteccion.sobreCalentamiento = false;
      proteccion.fugatDetectada = false;
      proteccion.bloqueoBomba = false;
      proteccion.riegosConsecutivos = 0;
      proteccion.nivelEmergencia = 0;
      proteccion.razonEmergencia = "";
      
      estadoActual = NORMAL;
      
      // Aplicar configuración del cultivo actual (esto ajustará algunos valores según el cultivo)
      aplicarConfiguracionCultivo();
      
      // tiempoMinimoEntreRiegos se ajusta en aplicarConfiguracionCultivo, 
      // pero si no hay cultivo, usar valor por defecto
      if (proteccion.tiempoMinimoEntreRiegos == 0) {
        proteccion.tiempoMinimoEntreRiegos = 60000;  // 1 minuto por defecto
      }
      
      message = "Sistema reiniciado";
      detalles = "Estadísticas y configuración reseteadas a valores iniciales. Cultivo actual: " + cultivos[cultivoSeleccionado].nombre;
      Serial.println("🔄 [SISTEMA] Sistema completamente reiniciado - Valores restaurados a iniciales");
      Serial.print("  Cultivo: ");
      Serial.println(cultivos[cultivoSeleccionado].nombre);
    } 
    else if (cmd == "regar") {
      // Riego manual
      if (proteccion.emergenciaActiva) {
        message = "No se puede regar";
        detalles = "El sistema está en modo emergencia. Resuelva primero la emergencia.";
        success = false;
      } else if (proteccion.bloqueoBomba) {
        message = "Bomba bloqueada";
        detalles = "Se detectó un bloqueo en la bomba. Verifique el sistema.";
        success = false;
      } else if (millis() - proteccion.ultimoRiego < proteccion.tiempoMinimoEntreRiegos) {
        unsigned long tiempoRestante = (proteccion.tiempoMinimoEntreRiegos - (millis() - proteccion.ultimoRiego)) / 1000;
        message = "Esperando tiempo mínimo";
        detalles = "Debe esperar " + String(tiempoRestante) + " segundos antes del próximo riego";
        success = false;
      } else if (digitalRead(RELE) == LOW) {
        digitalWrite(RELE, HIGH);
        proteccion.inicioRiego = millis();
        estadoActual = RIEGO_ACTIVO;
        tiempoInicioEstado = millis();
        message = "Riego manual iniciado";
        detalles = "La bomba está activa. El sistema detendrá el riego automáticamente si es necesario.";
        Serial.println("💧 [MANUAL] Riego manual iniciado");
      } else {
        message = "Riego ya activo";
        detalles = "La bomba ya está funcionando";
        success = false;
      }
    } 
    else if (cmd == "detener") {
      // Detener riego manualmente
      if (digitalRead(RELE) == HIGH) {
        digitalWrite(RELE, LOW);
        unsigned long duracionRiego = millis() - proteccion.inicioRiego;
        float aguaUsada = (flujoLitrosPorMinuto * duracionRiego) / 60000.0;
        proteccion.ultimoRiego = millis();
        estadoActual = NORMAL;
        message = "Riego detenido manualmente";
        detalles = "Duración: " + String(duracionRiego / 1000) + " segundos. Agua usada: " + String(aguaUsada, 2) + " litros";
        Serial.println("⏹️ [MANUAL] Riego detenido - Duración: " + String(duracionRiego / 1000) + "s");
      } else {
        message = "No hay riego activo";
        detalles = "La bomba ya está apagada";
        success = false;
      }
    } 
    else if (cmd == "reparar") {
      // AUTO-REPARACIÓN: Intentar reparar sensores
      autoReparacion.fallosDHT = 0;
      autoReparacion.intentosRecuperacionDHT = 0;
      autoReparacion.modoSeguro = false;
      dht.begin();
      delay(100);
      float testTemp = dht.readTemperature();
      float testHum = dht.readHumidity();
      if (!isnan(testTemp) && !isnan(testHum)) {
        autoReparacion.dhtOperativo = true;
        message = "Auto-reparación exitosa";
        detalles = "Sensor DHT recuperado. Temperatura: " + String(testTemp, 1) + "°C, Humedad: " + String(testHum, 1) + "%";
        Serial.println("🔧 [AUTO-REP] Sensor DHT recuperado exitosamente");
      } else {
        message = "Auto-reparación en proceso";
        detalles = "El sensor aún no responde. El sistema usará valores de respaldo.";
        Serial.println("⚠️ [AUTO-REP] Sensor DHT aún no responde, usando valores de respaldo");
      }
    }
    else {
      server.send(400, "application/json", "{\"status\":\"error\",\"message\":\"Comando desconocido\"}");
      return;
    }
    
    StaticJsonDocument<256> doc;
    doc["status"] = success ? "ok" : "warning";
    doc["message"] = message;
    doc["detalles"] = detalles;
    doc["timestamp"] = millis();
    
    String response;
    serializeJson(doc, response);
    server.send(200, "application/json", response);
  } else {
    server.send(400, "application/json", "{\"status\":\"error\",\"message\":\"Falta parametro cmd\"}");
  }
}

void handleGetHistorial() {
  setCORSHeaders();
  
  StaticJsonDocument<512> doc;
  JsonArray antes = doc.createNestedArray("humedadAntes");
  JsonArray despues = doc.createNestedArray("humedadDespues");
  
  for (int i = 0; i < 10; i++) {
    antes.add(optimizacion.humedadAntesRiego[i]);
    despues.add(optimizacion.humedadDespuesRiego[i]);
  }
  
  String response;
  serializeJson(doc, response);
  server.send(200, "application/json", response);
}

// ============================================
// SETUP
// ============================================
void setup() {
  Serial.begin(115200);
  Serial.println("\n\n╔════════════════════════════════════════╗");
  Serial.println("║  SISTEMA RIEGO AUTONOMICO v4.0       ║");
  Serial.println("║  NodeMCU ESP32 + API REST             ║");
  Serial.println("╚════════════════════════════════════════╝");
  
  // Configurar pines
  pinMode(RELE, OUTPUT);
  digitalWrite(RELE, LOW);
  pinMode(PIN_FLUJO, INPUT_PULLUP);
  
  // Configurar ADC para el sensor de suelo (ESP32)
  analogReadResolution(12);  // 12 bits (0-4095)
  analogSetAttenuation(ADC_11db);  // Rango completo 0-3.3V
  
  // Interrupciones en ESP32
  attachInterrupt(digitalPinToInterrupt(PIN_FLUJO), contarPulsos, RISING);
  
  dht.begin();
  
  aplicarConfiguracionCultivo();
  
  // Conectar WiFi
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  
  Serial.print("Conectando WiFi");
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 20) {
    delay(500);
    Serial.print(".");
    attempts++;
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\n✅ WiFi conectado");
    Serial.print("📡 IP: ");
    Serial.println(WiFi.localIP());
    
    ThingSpeak.begin(client);
    Serial.println("✅ ThingSpeak inicializado");
  } else {
    Serial.println("\n❌ No se pudo conectar al WiFi");
  }
  
  // Configurar endpoints REST
  server.on("/api/data", HTTP_GET, handleGetData);
  server.on("/api/cultivos", HTTP_GET, handleGetCultivos);
  server.on("/api/cultivo", HTTP_POST, handleSetCultivo);
  server.on("/api/command", HTTP_POST, handleCommand);
  server.on("/api/historial", HTTP_GET, handleGetHistorial);
  server.on("/api/config", HTTP_POST, handleSetConfig);
  
  // CORS preflight
  server.on("/api/data", HTTP_OPTIONS, handleOptions);
  server.on("/api/cultivos", HTTP_OPTIONS, handleOptions);
  server.on("/api/cultivo", HTTP_OPTIONS, handleOptions);
  server.on("/api/command", HTTP_OPTIONS, handleOptions);
  server.on("/api/historial", HTTP_OPTIONS, handleOptions);
  server.on("/api/config", HTTP_OPTIONS, handleOptions);
  
  server.begin();
  Serial.println("✅ API REST iniciada");
  Serial.println();
  Serial.println("════════════════════════════════════════");
  Serial.println("✓ Auto-configuración");
  Serial.println("✓ Auto-reparación");
  Serial.println("✓ Auto-optimización");
  Serial.println("✓ Auto-protección");
  Serial.println("════════════════════════════════════════");
  Serial.println();
  Serial.println("📱 Accede desde tu móvil a:");
  Serial.print("   http://");
  Serial.println(WiFi.localIP());
  Serial.println();
  Serial.println("🔄 Sistema iniciado correctamente");
  Serial.println();
  
  tiempoAntiguo = millis();
  tiempoInicioEstado = millis();
  estadoActual = CALIBRANDO;
}

// ============================================
// LOOP PRINCIPAL
// ============================================
void loop() {
  server.handleClient();
  
  if (millis() - tiempoAntiguo >= 1000) {
    detachInterrupt(digitalPinToInterrupt(PIN_FLUJO)); 
    flujoLitrosPorMinuto = (pulsoContador * 60.0 / factorK);
    pulsoContador = 0; 
    tiempoAntiguo = millis();
    attachInterrupt(digitalPinToInterrupt(PIN_FLUJO), contarPulsos, RISING);
  }
  
  float hAire = dht.readHumidity();
  float tAire = dht.readTemperature();
  int valorSuelo = analogRead(SENSOR_SUELO);
  
  if (!diagnosticarDHT(tAire, hAire)) {
    tAire = autoReparacion.ultimaTempValida;
    hAire = autoReparacion.ultimaHumAireValida;
    intentarRecuperarDHT();
  }
  
  temperaturaActual = tAire;
  humedadAireActual = hAire;
  
  // Mapear el valor del ESP32 (0-4095) a porcentaje
  int hSuelo = map(valorSuelo, config.sueloSecoMax, config.sueloHumedoMin, 0, 100);
  hSuelo = constrain(hSuelo, 0, 100);
  humedadSueloActual = hSuelo;
  
  switch (estadoActual) {
    case INICIANDO:
      if (millis() - tiempoInicioEstado > 5000) {
        estadoActual = CALIBRANDO;
        tiempoInicioEstado = millis();
      }
      break;
      
    case CALIBRANDO:
      autoCalibracion();
      if (config.calibracionCompletada) {
        estadoActual = NORMAL;
        tiempoInicioEstado = millis();
      }
      break;
      
    case NORMAL:
    case RIEGO_ACTIVO:
      if (proteccion.emergenciaActiva) {
        manejarEmergencia();
        break;
      }
      diagnosticarFlujo();
      controlarRiego(hSuelo, tAire);
      optimizarParametros();
      break;
      
    case EMERGENCIA:
      manejarEmergencia();
      break;
  }
  
  if (WiFi.status() == WL_CONNECTED && channelID > 0) {
    enviarDatosThingSpeak();
  }
  
  delay(100);
}
