# Documentación Exhaustiva: Principios de Automatización Autónoma
## Sistema de Riego Inteligente - NodeMCU ESP32

---

## INTRODUCCIÓN

Este documento describe en detalle la implementación de los cuatro principios fundamentales de automatización autónoma en el sistema de riego inteligente. Cada principio está diseñado para que el sistema opere de manera independiente, adaptándose a las condiciones del entorno sin intervención humana constante.

---

## PRINCIPIO 1: AUTOCONFIGURACIÓN (Self-Configuration)

### Descripción General

La **autoconfiguración** permite que el sistema detecte automáticamente las características del entorno físico (tipo de suelo, condiciones ambientales) y ajuste sus parámetros operativos sin necesidad de configuración manual previa.

### Implementación Técnica

#### 1.1 Calibración Automática del Sensor de Humedad del Suelo

**¿Qué se realiza?**

El sistema ejecuta un proceso de calibración automática durante los primeros 30 segundos de operación. Este proceso:

- **Recopila lecturas continuas** del sensor analógico de humedad del suelo (GPIO34, ADC de 12 bits, rango 0-4095)
- **Determina los valores extremos** (máximo para suelo seco, mínimo para suelo húmedo) mediante muestreo estadístico
- **Almacena estos valores** en las variables `config.sueloSecoMax` y `config.sueloHumedoMin`
- **Valida las lecturas** descartando valores fuera del rango válido (0-4095)

**¿Por qué es importante?**

Cada sensor de humedad tiene características diferentes y cada tipo de suelo presenta resistencias distintas. La calibración automática permite:
- Adaptarse a las variaciones de fabricación del sensor
- Compensar las diferencias entre tipos de suelo
- Establecer un mapeo preciso entre valores analógicos y porcentajes de humedad

**Código relacionado:**
```cpp
void autoCalibracion() {
  // Recopila lecturas durante 30 segundos
  // Determina rangos máximo y mínimo
  // Clasifica el tipo de suelo
}
```

#### 1.2 Detección Automática del Tipo de Suelo

**¿Qué se realiza?**

Mediante análisis de variabilidad estadística de las lecturas del sensor, el sistema clasifica automáticamente el tipo de suelo en tres categorías:

- **Suelo Arcilloso**: Baja variabilidad (< 200 unidades ADC)
  - Características: Retiene agua eficientemente, drenaje lento
  - Ajuste: Tiempo de riego base = 45 segundos
  
- **Suelo Arenoso**: Alta variabilidad (> 600 unidades ADC)
  - Características: Drena rápidamente, requiere riegos más frecuentes
  - Ajuste: Tiempo de riego base = 20 segundos
  
- **Suelo Limoso**: Variabilidad media (200-600 unidades ADC)
  - Características: Balance entre retención y drenaje
  - Ajuste: Tiempo de riego base = 30 segundos

**¿Por qué es importante?**

El tipo de suelo determina:
- La velocidad de absorción del agua
- La frecuencia necesaria de riego
- La duración óptima de cada ciclo
- El riesgo de encharcamiento

**Algoritmo de clasificación:**
```cpp
int variacion = variacionMax - variacionMin;
if (variacion < 200) {
  // Suelo arcilloso - riegos más largos
} else if (variacion > 600) {
  // Suelo arenoso - riegos más cortos y frecuentes
} else {
  // Suelo limoso - riegos moderados
}
```

#### 1.3 Ajuste Automático de Parámetros según Cultivo

**¿Qué se realiza?**

Cuando el usuario selecciona un cultivo, el sistema automáticamente:

- **Ajusta los umbrales de humedad** (mínima, óptima, máxima) según las necesidades del cultivo
- **Aplica un factor de multiplicación** (`factorAgua`) que modifica la cantidad de agua necesaria
- **Configura el tiempo mínimo entre riegos** basado en la frecuencia recomendada para ese cultivo
- **Actualiza todos los parámetros operativos** sin requerir configuración manual adicional

**Ejemplo de aplicación:**
- Cultivo: Tomate → Humedad mínima: 60%, Factor agua: 1.2x, Frecuencia: cada 60 minutos
- Cultivo: Cebolla → Humedad mínima: 40%, Factor agua: 0.8x, Frecuencia: cada 100 minutos

**¿Por qué es importante?**

Diferentes cultivos tienen necesidades hídricas distintas. La autoconfiguración garantiza que el sistema opere con parámetros óptimos para cada tipo de planta sin intervención del usuario.

---

## PRINCIPIO 2: AUTOOPTIMIZACIÓN (Self-Optimization)

### Descripción General

La **autooptimización** permite que el sistema mejore continuamente su rendimiento mediante el análisis de datos históricos y el ajuste dinámico de parámetros operativos para maximizar la eficiencia del uso del agua.

### Implementación Técnica

#### 2.1 Registro y Análisis de Ciclos de Riego

**¿Qué se realiza?**

El sistema mantiene un historial de los últimos 10 ciclos de riego, registrando para cada uno:

- **Humedad del suelo antes del riego**: Valor inicial que determina la necesidad de riego
- **Humedad del suelo después del riego**: Valor final que indica la efectividad
- **Agua utilizada**: Calculada mediante el sensor de flujo (litros por minuto × duración)
- **Duración del riego**: Tiempo en milisegundos que la bomba estuvo activa
- **Tasa de absorción**: Incremento de humedad por segundo de riego

**Cálculo de eficiencia:**
```cpp
eficiencia = (humedadDespues - humedadAntes) / aguaUsada * 100
```

**¿Por qué es importante?**

Este registro permite:
- Identificar patrones de comportamiento del suelo
- Detectar si el tiempo de riego es insuficiente o excesivo
- Calcular la eficiencia real del sistema
- Ajustar parámetros basándose en datos empíricos

#### 2.2 Algoritmo de Aprendizaje y Ajuste Dinámico

**¿Qué se realiza?**

Cada 10 ciclos de riego, el sistema ejecuta un proceso de optimización:

1. **Cálculo de eficiencia promedio**: Analiza los últimos 10 ciclos para determinar la eficiencia media
2. **Identificación del tiempo óptimo**: Compara la eficiencia de diferentes duraciones de riego
3. **Ajuste progresivo**: Si la eficiencia actual es menor que la mejor registrada, ajusta el tiempo de riego hacia el valor óptimo histórico
4. **Convergencia gradual**: El ajuste se realiza mediante promedio ponderado para evitar cambios bruscos

**Algoritmo de optimización:**
```cpp
if (eficienciaActual > mejorEficiencia) {
  // Guardar nuevo tiempo óptimo
  tiempoOptimo = tiempoRiegoActual;
} else {
  // Ajustar hacia el tiempo óptimo
  tiempoRiegoActual = (tiempoRiegoActual + tiempoOptimo) / 2;
}
```

**¿Por qué es importante?**

- **Maximiza la eficiencia del agua**: Reduce el desperdicio ajustando la duración exacta necesaria
- **Adaptación continua**: Se ajusta a cambios estacionales o en las condiciones del suelo
- **Aprendizaje incremental**: Mejora con el tiempo sin requerir intervención

#### 2.3 Optimización Basada en Temperatura Ambiental

**¿Qué se realiza?**

El sistema ajusta automáticamente los umbrales de humedad según la temperatura:

- **Temperatura > 30°C**: Factor de temperatura = 1.3x
  - Razón: Mayor evaporación, el suelo se seca más rápido
  - Efecto: Riegos más frecuentes o de mayor duración
  
- **Temperatura < 15°C**: Factor de temperatura = 0.7x
  - Razón: Menor evaporación, el suelo retiene humedad más tiempo
  - Efecto: Riegos menos frecuentes
  
- **Temperatura 15-30°C**: Factor de temperatura = 1.0x
  - Razón: Condiciones normales
  - Efecto: Parámetros estándar

**Cálculo del umbral ajustado:**
```cpp
umbralAjustado = umbralHumedadMin * factorTemperatura * factorCultivo
```

**¿Por qué es importante?**

Las condiciones ambientales varían constantemente. Esta optimización permite:
- Adaptarse a cambios diarios de temperatura
- Compensar la evaporación estacional
- Mantener condiciones óptimas independientemente del clima

#### 2.4 Análisis de Tasa de Absorción del Suelo

**¿Qué se realiza?**

El sistema calcula continuamente la velocidad a la que el suelo absorbe agua:

```cpp
tasaAbsorcion = (humedadDespues - humedadAntes) / (tiempoRiego / 1000.0)
```

Este valor indica:
- **Alta tasa (> 2% por segundo)**: Suelo muy seco o muy poroso, puede necesitar más agua
- **Baja tasa (< 0.5% por segundo)**: Suelo saturado o compacto, riesgo de encharcamiento
- **Tasa normal (0.5-2% por segundo)**: Condiciones ideales

**¿Por qué es importante?**

La tasa de absorción permite:
- Detectar saturación antes de que ocurra
- Ajustar la duración del riego en tiempo real
- Prevenir encharcamiento y desperdicio de agua

---

## PRINCIPIO 3: AUTOCURACIÓN (Self-Healing)

### Descripción General

La **autocuración** permite que el sistema detecte fallos en sus componentes, implemente medidas de respaldo y recupere automáticamente la funcionalidad sin intervención humana.

### Implementación Técnica

#### 3.1 Diagnóstico Continuo del Sensor DHT22

**¿Qué se realiza?**

El sistema monitorea constantemente la integridad del sensor DHT22 (temperatura y humedad del aire):

1. **Validación de lecturas**: Verifica que los valores no sean NaN (Not a Number)
2. **Contador de fallos**: Incrementa un contador cada vez que se detecta una lectura inválida
3. **Umbral de fallo**: Después de 3 fallos consecutivos, marca el sensor como no operativo
4. **Sistema de respaldo**: Utiliza los últimos valores válidos cuando el sensor falla

**Código de diagnóstico:**
```cpp
bool diagnosticarDHT(float temp, float hum) {
  if (isnan(temp) || isnan(hum)) {
    fallosDHT++;
    if (fallosDHT >= 3) {
      dhtOperativo = false;
      return false;
    }
  } else {
    fallosDHT = 0;  // Reset si lectura es válida
    dhtOperativo = true;
  }
}
```

**¿Por qué es importante?**

El sensor DHT22 puede fallar por:
- Condensación de humedad
- Conexiones sueltas
- Desgaste del componente
- Interferencias eléctricas

La autocuración garantiza que el sistema continúe operando incluso con sensores defectuosos.

#### 3.2 Sistema de Valores de Respaldo

**¿Qué se realiza?**

Cuando el sensor DHT22 falla, el sistema:

- **Almacena los últimos valores válidos** antes del fallo
- **Utiliza estos valores** para todas las operaciones que requieren temperatura/humedad
- **Continúa operando normalmente** sin interrupciones
- **Marca el sistema en "modo seguro"** para indicar que está usando valores de respaldo

**Estructura de respaldo:**
```cpp
struct SistemaAutoReparacion {
  float ultimaTempValida = 25.0;      // Última temperatura válida
  float ultimaHumAireValida = 50.0;   // Última humedad válida
  int ultimaHumSueloValida = 50;      // Última humedad de suelo válida
}
```

**¿Por qué es importante?**

- **Continuidad del servicio**: El sistema no se detiene por un fallo de sensor
- **Prevención de daños**: Evita decisiones erróneas basadas en lecturas inválidas
- **Transparencia**: Informa al usuario que está en modo seguro

#### 3.3 Intentos Automáticos de Recuperación

**¿Qué se realiza?**

Cada 30 segundos, el sistema intenta recuperar el sensor DHT22:

1. **Reinicialización**: Llama a `dht.begin()` para resetear el sensor
2. **Prueba de lectura**: Intenta leer temperatura y humedad
3. **Validación**: Si las lecturas son válidas, restaura el estado operativo
4. **Límite de intentos**: Después de 5 intentos fallidos, activa modo seguro permanente

**Código de recuperación:**
```cpp
void intentarRecuperarDHT() {
  if (millis() - ultimoIntentoRecuperacion > 30000) {
    dht.begin();  // Reinicializar sensor
    intentosRecuperacionDHT++;
    if (intentosRecuperacionDHT > 5) {
      modoSeguro = true;  // Activar modo seguro permanente
    }
  }
}
```

**¿Por qué es importante?**

Muchos fallos de sensores son temporales:
- Problemas de conexión que se resuelven solos
- Condensación que se evapora
- Interferencias momentáneas

Los intentos automáticos permiten recuperar el sensor sin intervención manual.

#### 3.4 Diagnóstico del Sensor de Flujo

**¿Qué se realiza?**

El sistema monitorea continuamente el sensor de flujo para detectar:

- **Bloqueo de bomba**: Si la bomba está activa pero no hay flujo de agua después de 5 segundos
- **Fuga de agua**: Si la bomba está apagada pero se detecta flujo de agua

**Código de diagnóstico:**
```cpp
bool diagnosticarFlujo() {
  if (digitalRead(RELE) == HIGH && flujoLitrosPorMinuto < 0.1 && 
      millis() - inicioRiego > 5000) {
    bloqueoBomba = true;  // Bomba activa sin flujo
    return false;
  }
  if (digitalRead(RELE) == LOW && flujoLitrosPorMinuto > 0.5) {
    fugaDetectada = true;  // Flujo sin bomba activa
    return false;
  }
  return true;
}
```

**¿Por qué es importante?**

- **Protección de la bomba**: Detecta bloqueos antes de que causen daño
- **Detección de fugas**: Identifica pérdidas de agua que aumentan costos
- **Mantenimiento predictivo**: Permite intervención antes de fallos mayores

---

## PRINCIPIO 4: AUTOPROTECCIÓN (Self-Protection)

### Descripción General

La **autoprotección** permite que el sistema detecte condiciones peligrosas, implemente medidas de seguridad automáticas y se recupere de situaciones de emergencia sin intervención externa.

### Implementación Técnica

#### 4.1 Sistema de Verificación de Condiciones Seguras

**¿Qué se realiza?**

Antes de iniciar cualquier riego, el sistema verifica múltiples condiciones de seguridad:

1. **Temperatura máxima**: Si la temperatura excede 45°C, bloquea el riego
   - Razón: Riesgo de daño a plantas y componentes
   - Acción: Activa emergencia por sobrecalentamiento

2. **Temperatura mínima**: Si la temperatura es menor a 0°C, bloquea el riego
   - Razón: Riesgo de congelación del agua y daño a tuberías
   - Acción: Activa emergencia por riesgo de congelación

3. **Tiempo máximo de riego**: Si un riego excede 5 minutos continuos, lo detiene automáticamente
   - Razón: Prevenir encharcamiento y desperdicio de agua
   - Acción: Detiene bomba y activa emergencia

4. **Humedad máxima del suelo**: Si la humedad supera el umbral máximo, bloquea nuevos riegos
   - Razón: Prevenir saturación y daño a raíces
   - Acción: Activa emergencia por sobresaturación

5. **Tiempo mínimo entre riegos**: Verifica que haya pasado el tiempo mínimo configurado
   - Razón: Permitir que el suelo absorba el agua antes del próximo riego
   - Acción: Bloquea el riego hasta que pase el tiempo requerido

**Código de verificación:**
```cpp
bool verificarCondicionesSeguras(float temp, int humedadSuelo) {
  if (temp > tempMaxSegura) return false;  // Sobrecalentamiento
  if (temp < tempMinSegura) return false;   // Riesgo de congelación
  if (tiempoRiego > tiempoMaxRiegoContinuo) return false;  // Tiempo excedido
  if (humedadSuelo > umbralHumedadMax) return false;  // Sobresaturación
  if (tiempoDesdeUltimoRiego < tiempoMinimoEntreRiegos) return false;  // Frecuencia excesiva
  return true;
}
```

**¿Por qué es importante?**

Estas verificaciones previenen:
- Daño a las plantas por condiciones extremas
- Desperdicio de agua por riegos excesivos
- Daño a componentes del sistema
- Condiciones que podrían requerir reparación costosa

#### 4.2 Sistema de Gestión de Emergencias

**¿Qué se realiza?**

Cuando se detecta una condición peligrosa, el sistema:

1. **Detiene inmediatamente** la bomba de riego
2. **Cambia el estado** a EMERGENCIA
3. **Registra la razón** de la emergencia para diagnóstico
4. **Activa modo de recuperación automática** con temporizador de 60 segundos
5. **Informa al usuario** mediante la API REST sobre el estado de emergencia

**Estados de emergencia:**
- **Nivel 0**: Normal, sin emergencias
- **Nivel 1**: Advertencia, condiciones subóptimas pero operativas
- **Nivel 2**: Crítica, sistema detenido por seguridad

**Código de manejo de emergencia:**
```cpp
void manejarEmergencia() {
  digitalWrite(RELE, LOW);  // Detener bomba inmediatamente
  estadoActual = EMERGENCIA;
  
  // Auto-recuperación después de 60 segundos
  if (millis() - tiempoInicioEmergencia > 60000) {
    // Restablecer todas las condiciones de emergencia
    emergenciaActiva = false;
    estadoActual = NORMAL;
  }
}
```

**¿Por qué es importante?**

- **Respuesta inmediata**: Previene daños antes de que ocurran
- **Recuperación automática**: No requiere intervención manual
- **Transparencia**: El usuario siempre sabe el estado del sistema
- **Prevención de cascadas**: Evita que un problema cause múltiples fallos

#### 4.3 Protección contra Riegos Consecutivos Excesivos

**¿Qué se realiza?**

El sistema rastrea el número de riegos consecutivos y:

- **Limita la frecuencia**: Aplica un tiempo mínimo obligatorio entre riegos
- **Monitorea patrones**: Detecta si hay demasiados riegos en poco tiempo
- **Ajusta dinámicamente**: Puede aumentar el tiempo mínimo si detecta abuso

**¿Por qué es importante?**

Riegos demasiado frecuentes pueden:
- Causar encharcamiento
- Lavar nutrientes del suelo
- Dañar las raíces de las plantas
- Desperdiciar agua y energía

#### 4.4 Protección contra Bloqueo de Bomba

**¿Qué se realiza?**

El sistema detecta cuando la bomba está activa pero no hay flujo de agua:

- **Detección**: Compara el estado del relé con las lecturas del sensor de flujo
- **Tiempo de espera**: Permite 5 segundos de margen para el arranque de la bomba
- **Acción**: Si después de 5 segundos no hay flujo, marca la bomba como bloqueada
- **Bloqueo**: Impide nuevos intentos de riego hasta que se resuelva

**¿Por qué es importante?**

Un bloqueo de bomba puede:
- Causar sobrecalentamiento y daño permanente
- Consumir energía sin beneficio
- Requerir reparación costosa
- Interrumpir el servicio por períodos prolongados

#### 4.5 Detección de Fugas de Agua

**¿Qué se realiza?**

El sistema detecta flujo de agua cuando la bomba debería estar apagada:

- **Monitoreo continuo**: Compara el estado del relé con el sensor de flujo
- **Umbral de detección**: Considera fuga si hay más de 0.5 L/min sin bomba activa
- **Acción inmediata**: Activa emergencia y notifica al usuario
- **Registro**: Guarda el evento para análisis posterior

**¿Por qué es importante?**

Las fugas pueden:
- Desperdiciar grandes cantidades de agua
- Aumentar costos operativos significativamente
- Indicar daño en tuberías o conexiones
- Requerir reparación urgente

---

## INTEGRACIÓN DE LOS PRINCIPIOS

### Flujo de Operación Integrado

El sistema opera integrando los cuatro principios de manera simultánea:

1. **Al iniciar**: Autoconfiguración (calibración automática)
2. **Durante operación normal**: 
   - Autooptimización (ajuste continuo de parámetros)
   - Autocuración (monitoreo y recuperación de sensores)
   - Autoprotección (verificación continua de condiciones seguras)
3. **En caso de fallo**: Autocuración y autoprotección trabajan juntos
4. **Después de fallo**: Autoconfiguración puede recalibrar si es necesario

### Ejemplo de Operación Integrada

**Escenario**: Sensor DHT22 falla durante un día caluroso

1. **Autocuración detecta el fallo** → Usa valores de respaldo
2. **Autoprotección verifica temperatura** → Usa último valor válido (30°C)
3. **Autooptimización ajusta umbrales** → Aplica factor de temperatura 1.3x
4. **Sistema continúa operando** → Sin interrupciones
5. **Autocuración intenta recuperar** → Cada 30 segundos
6. **Cuando se recupera** → Vuelve a modo normal automáticamente

---

## MÉTRICAS Y MONITOREO

### Indicadores de Rendimiento

El sistema proporciona métricas para evaluar la efectividad de cada principio:

**Autoconfiguración:**
- Tiempo de calibración completada
- Tipo de suelo detectado
- Precisión del mapeo de humedad

**Autooptimización:**
- Eficiencia promedio del sistema
- Tiempo óptimo de riego determinado
- Tasa de mejora continua

**Autocuración:**
- Tasa de recuperación de sensores
- Tiempo en modo seguro
- Número de fallos detectados y resueltos

**Autoprotección:**
- Número de emergencias detectadas
- Tiempo de respuesta a emergencias
- Tasa de recuperación automática

---

## CONCLUSIONES

La implementación de estos cuatro principios de automatización autónoma permite que el sistema de riego opere de manera completamente independiente, adaptándose a las condiciones del entorno, optimizando su rendimiento, recuperándose de fallos y protegiéndose de condiciones peligrosas, todo sin requerir intervención humana constante.

Este enfoque garantiza:
- **Confiabilidad**: El sistema continúa operando incluso con fallos parciales
- **Eficiencia**: Optimización continua del uso de recursos
- **Seguridad**: Protección automática contra condiciones peligrosas
- **Mantenibilidad**: Diagnóstico y recuperación automáticos reducen la necesidad de intervención

---

*Documento generado para informe técnico del Sistema de Riego Autónomo v4.0*

