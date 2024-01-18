# SIPRI–Valdepeñas: Sistema de Prevención de Inundaciones

## Descripción

El SIPRI–Valdepeñas es un *Sistema de Prevención de Inundaciones* diseñado para abordar el persistente problema de inundaciones en Valdepeñas, especialmente durante períodos de condiciones climáticas extremas, que resultan en significativos daños económicos y, en ocasiones, afectan a la seguridad humana. La propuesta se centra en el desarrollo de una aplicación basada en el *Internet de las cosas (IoT)* para monitorear y controlar el impacto de la catástrofe, al mismo tiempo que impulsa la ayuda al sector agrícola.

## Configuración

Se ha utilizado el Stack *TIG* (*Telegraf, InfluxDB y Grafana*) para la configuración del entorno, desplegándose en un servidor cuyo dominio es [ciberfisicos.ddns.net](http://ciberfisicos.ddns.net). 

Para acceder al panel de Grafana, utiliza la siguiente URL: [http://ciberfisicos.ddns.net:3000](http://ciberfisicos.ddns.net:3000).

## Casos de Uso

Hay 3 casos de uso.

### Caso de Uso 1: Subida y bajada de la tapa de la alcantarilla

Si la humedad es mayor o igual a un valor predefinido (en este caso, 70), se elevará la tapa si está bajada. Para lograr esto, se utilizará un *servo* que variará su *duty_cycle* lentamente de 2.5 a 12.5. En caso de que la humedad sea inferior al umbral, se bajará la tapa si está elevada, representándose con el servo con un *duty_cycle* que irá de 12.5 a 2.5.

#### Paneles

Se han creado 6 paneles en Grafana para monitorizar y analizar el sistema de control de la tapa en función de la humedad:

1. **Valor Actual de la Humedad:**
   - Proporciona la lectura actual de la humedad en tiempo real.

2. **Histórico de Valores de la Humedad:**
   - Muestra un registro histórico de los valores de humedad a lo largo del tiempo.

3. **Estado del Motor:**
   - Indica si el motor está encendido o apagado en el momento actual.

4. **Estado de la Tapa:**
   - Refleja el estado actual de la tapa, que puede estar subida, bajada, subiendo o bajando.

5. **Histórico de Cambios en el Estado del Motor:**
   - Registra los cambios en el estado del motor a lo largo del tiempo.

6. **Histórico de Cambios en el Estado de la Tapa:**
   - Registra los cambios en el estado de la tapa a lo largo del tiempo.

### Caso de Uso 2: Congelamiento de la alcantarilla

Se permite cambiar el umbral de temperatura (*A0*). Si la temperatura está por debajo del umbral, se enciende el *LED D13* (azul); de lo contrario, se enciende el *LED D12* (rojo).

#### Paneles

Se han configurado 3 paneles en Grafana para facilitar la supervisión del sistema de control de temperatura:

1. **Valor Actual de la Temperatura:**
   - Muestra la temperatura actual en tiempo real.

2. **Valor Actual del Umbral de Temperatura:**
   - Indica el umbral de temperatura actualmente establecido.

3. **Histórico de Valores de la Temperatura:**
   - Proporciona un historial de los valores de temperatura a lo largo del tiempo.

### Caso de Uso 3: Obstrucción de la alcantarilla

Se utiliza el sensor de luz *A1* para detectar poca luz (por debajo del umbral predefinido de 500). Se enciende el *LED RGB* en blanco para indicar obstrucción; de lo contrario, se apaga el *LED*.

#### Paneles

Se han diseñado 3 paneles en Grafana para facilitar la supervisión del sistema de detección de obstrucción por luz:

1. **Valor Actual de la Luz:**
   - Muestra el valor actual de luz en tiempo real.

2. **Indicador de Obstrucción:**
   - Indica si hay obstrucción o no, basándose en la detección de la luz.

3. **Histórico de Valores de Luz:**
   - Ofrece un historial detallado de los valores de luz a lo largo del tiempo.
