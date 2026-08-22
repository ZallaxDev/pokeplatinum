# Pokémon Platinum Native 3DS Port — Plan de implementación para Codex

## 0. Objetivo

Este documento define el plan de trabajo para portar **`pret/pokeplatinum`** a Nintendo 3DS como aplicación **homebrew nativa ARM11**, utilizando el código decompilado de Pokémon Platinum como base.

Cuando leas referencias a codex, se refiere a ti mismo.

Repositorio upstream:

- https://github.com/pret/pokeplatinum

Toolchain 3DS:

- devkitPro / devkitARM
- libctru
- citro3d
- citro2d
- ndsp
- romfs

Referencias oficiales útiles:

- https://github.com/devkitPro/libctru
- https://github.com/devkitPro/citro3d
- https://github.com/devkitPro/citro2d
- https://github.com/devkitPro/3ds-examples
- https://devkitpro.org/wiki/Getting_Started

---

# 1. Alcance del proyecto

El objetivo de esta primera etapa NO es crear una edición mejorada de Pokémon Platinum.

El objetivo es conseguir una versión de Pokémon Platinum que:

- compile como software nativo de Nintendo 3DS;
- genere inicialmente un `.3dsx`;
- arranque desde Homebrew Launcher;
- ejecute la lógica original de Pokémon Platinum;
- pueda cargar los recursos necesarios del juego;
- acepte input de 3DS;
- reproduzca los gráficos necesarios para jugar;
- reproduzca audio;
- permita comenzar una partida;
- permita caminar por el mundo;
- permita ejecutar scripts y eventos;
- permita entrar y salir de edificios;
- permita abrir menús;
- permita combatir;
- permita guardar y cargar;
- permita completar una partida normal sin depender de emulación de Nintendo DS.

La prioridad es **compatibilidad funcional**, no fidelidad binaria.

No se intentará que el binario 3DS coincida con ningún binario original de Nintendo DS.

---

# 2. Determinación de assets.

Es posible que la compilación original requiera de una rom .nds para extraer los assets. Si es el caso, solicítalo una única vez al usuario para extraerlos y deja preparado lo necesario para continuar trabajando.

---

# 3. Principio arquitectónico

NO intentar convertir el proyecto completo de Nintendo DS a 3DS mediante sustituciones indiscriminadas.

La arquitectura debe evolucionar hacia:

```text
                    pokeplatinum
                         |
             +-----------+-----------+
             |                       |
        game logic               platform API
             |                       |
             |               +-------+-------+
             |               |               |
             |             NDS             3DS
             |          original         native
             |               |               |
             |          NitroSDK-ish      libctru
             |                            citro2d
             |                            citro3d
             |                            ndsp
             |                            romfs
             |
       gameplay systems
```

El objetivo es separar progresivamente:

## Código independiente de plataforma

Ejemplos:

- Pokémon;
- estadísticas;
- movimientos;
- objetos;
- lógica de combate;
- scripts;
- flags;
- eventos;
- inventario;
- entrenadores;
- guardado lógico;
- lógica de mapas;
- IA;
- RNG cuando sea posible;
- lógica de menús.

## Código dependiente de plataforma

Ejemplos:

- GPU;
- VRAM;
- BG;
- OAM;
- sprites;
- texturas;
- filesystem;
- DMA;
- timers;
- interrupts;
- input;
- audio;
- memoria específica de DS;
- comunicación ARM7/ARM9;
- NitroSDK;
- sistema de overlays;
- hardware GX/G2;
- servicios propios de Nintendo DS.

---


# PRINCIPIO CLAVE — Cada tarea debe producir una prueba visible

El roadmap NO debe contener tareas cuya finalización sólo pueda verificarse leyendo código.

Cada bulletpoint de `roadmap.md` debe incluir un **criterio de aceptación verificable**.

Preferencia de verificación, en este orden:

1. comportamiento observable al abrir el `.3dsx` en emulador;
2. mensaje visible en pantalla;
3. cambio de estado visible dentro del juego;
4. log verificable;
5. test automatizado cuando no exista una manifestación visual razonable.

Formato recomendado:

```md
- [ ] Inicializar libctru.
  - Verify: el `.3dsx` arranca y muestra `Platform init OK` en la pantalla inferior.

- [ ] Implementar input A/B.
  - Verify: al pulsar A/B en el emulador, la pantalla de debug muestra el estado correcto.

- [ ] Implementar carga de recursos.
  - Verify: cargar y mostrar una textura real de Pokémon Platinum en la pantalla superior.

- [ ] Implementar movimiento.
  - Verify: el protagonista puede desplazarse por un mapa real usando el D-Pad.
```

Evitar bullets como:

```md
- [ ] Portar filesystem.
- [ ] Migrar renderer.
- [ ] Arreglar memoria.
```

Deben descomponerse en objetivos observables:

```md
- [ ] Abrir desde romfs un archivo de prueba.
  - Verify: mostrar su tamaño y checksum en debug.

- [ ] Cargar una textura real desde los assets de Platinum.
  - Verify: verla correctamente en pantalla.

- [ ] Crear el heap principal.
  - Verify: reservar, escribir, leer y liberar memoria y mostrar `HEAP TEST OK`.
```

Cada milestone debe poder enseñarse en emulador como una pequeña demo acumulativa del progreso.

---

# Sistema de depuración obligatorio

El port debe disponer desde el principio de una capa propia de debug.

Crear una interfaz parecida a:

```c
void Debug_Init(void);
void Debug_Shutdown(void);

void Debug_Log(const char *format, ...);
void Debug_Warn(const char *format, ...);
void Debug_Error(const char *format, ...);

void Debug_SetOverlayEnabled(bool enabled);
bool Debug_IsOverlayEnabled(void);
```

El sistema debe soportar varios backends sin acoplar el gameplay a ninguno.

Arquitectura:

```text
game / platform code
        |
        v
     Debug API
        |
   +----+------------------+
   |                       |
external logging      bottom-screen overlay
```

Siempre que el emulador o entorno permita logs externos fiables, utilizarlos.

Cuando no haya una vía directa o cómoda para inspeccionar logs, utilizar la **pantalla inferior como consola de depuración**.

---

# Consola de depuración en pantalla inferior

Durante las fases tempranas del port, la pantalla inferior puede reservarse para debug.

Debe poder mostrar:

- estado del último milestone;
- mensajes INFO;
- warnings;
- errores;
- FPS;
- frame counter;
- memoria utilizada;
- mapa actual;
- coordenadas;
- último botón recibido;
- recurso que se está cargando;
- función/subsistema actual;
- error fatal antes de abortar.

Ejemplo:

```text
POKEPLATINUM 3DS DEBUG

BOOT        OK
ROMFS       OK
INPUT       OK
RENDERER    OK

Map: 0012
Player: 14, 27
Frame: 18231
FPS: 59.8

Last:
Loaded area_data.narc
```

No es necesario implementar una terminal completa.

Debe priorizar legibilidad y robustez.

---

# Debug overlay completamente desactivable

La consola inferior NO debe convertirse en una dependencia del juego.

Debe existir una opción de compilación clara:

```text
PORT3DS_DEBUG_OVERLAY=1
```

o equivalente.

Ejemplo:

```make
make 3ds DEBUG=1
```

puede activar:

```text
debug logs
bottom-screen debug overlay
assert information
extra diagnostics
```

Mientras:

```make
make 3ds RELEASE=1
```

debe poder compilar con el overlay completamente eliminado o deshabilitado.

Objetivo:

```text
DEBUG build
top screen    -> game
bottom screen -> debug console

RELEASE build
top screen    -> game
bottom screen -> original Pokémon Platinum UI
```

Cuando el port llegue al punto en que Platinum necesite utilizar realmente la pantalla inferior, el overlay debe poder alternarse durante desarrollo mediante una combinación de botones.

Sugerencia inicial:

```text
L + R + SELECT
```

para:

```text
original bottom screen
        ↕
debug overlay
```

La combinación exacta puede cambiar si interfiere con el gameplay.

Debe documentarse en `SPECS.md`.

La versión release no debe interceptar dicha combinación.

---

# Ring buffer de logs

La consola visual no debe depender de imprimir inmediatamente cada mensaje.

Implementar un pequeño ring buffer.

Ejemplo conceptual:

```c
#define DEBUG_LOG_LINES 64
#define DEBUG_LOG_LINE_LENGTH 96
```

Guardar las últimas líneas de log.

La pantalla inferior sólo renderiza las N últimas.

Esto permite:

- consultar errores recientes;
- evitar allocs constantes;
- mantener logs aunque el overlay estuviera oculto;
- mostrar el overlay después de que ocurra un problema.

No optimizar prematuramente los tamaños exactos.

---

# Errores fatales

En build DEBUG, una condición fatal debe intentar mostrar:

```text
FATAL ERROR

Subsystem: Renderer
Function: LoadTexture
Error: unsupported texture format
Asset: field/land_data/012

Press START to exit.
```

antes de cerrar.

Evitar crashes silenciosos siempre que el estado del sistema permita dibujar el error.

---

# Roadmap orientado a demos verificables

Codex debe diseñar `roadmap.md` como una secuencia de demos acumulativas.

Ejemplo de granularidad esperada:

```md
## Phase 0 — Toolchain

- [ ] Instalar dependencias host.
  - Verify: `make` puede ejecutar las herramientas host requeridas.

- [ ] Compilar el upstream original.
  - Verify: `build/pokeplatinum.us.nds` existe y su SHA-1 coincide.

- [ ] Instalar devkitPro y 3ds-dev.
  - Verify: compilar el hello-world oficial.

- [ ] Crear primer target 3DS.
  - Verify: abrir `pokeplatinum-3ds.3dsx` y ver `BOOT OK`.

## Phase 1 — Debug foundation

- [ ] Crear Debug API.
  - Verify: mostrar `Debug_Init OK` en la pantalla inferior.

- [ ] Añadir ring buffer.
  - Verify: generar más líneas que la pantalla y comprobar scroll/recorte correcto.

- [ ] Añadir toggle del overlay.
  - Verify: `L+R+SELECT` oculta y muestra la consola.

- [ ] Añadir build sin overlay.
  - Verify: build release arranca sin consola ni combinación de debug.

## Phase 2 — Input

- [ ] Leer D-Pad.
  - Verify: mostrar `UP/DOWN/LEFT/RIGHT` en tiempo real.

- [ ] Leer botones A/B/X/Y.
  - Verify: cada botón cambia un indicador visible.

- [ ] Leer touch.
  - Verify: mostrar coordenadas táctiles en la consola.

## Phase 3 — Filesystem

- [ ] Montar romfs.
  - Verify: `ROMFS OK`.

- [ ] Abrir archivo empaquetado.
  - Verify: mostrar nombre, tamaño y checksum.

- [ ] Leer un recurso real de Platinum.
  - Verify: checksum comparado contra el resultado esperado.

## Phase 4 — Renderer

- [ ] Inicializar GPU.
  - Verify: pantalla superior cambia a un color de prueba y aparece `GPU OK`.

- [ ] Renderizar una textura.
  - Verify: textura de prueba visible.

- [ ] Renderizar un asset real de Platinum.
  - Verify: sprite/imagen real visible y sin corrupción.

- [ ] Dibujar múltiples sprites.
  - Verify: cuatro sprites en posiciones diferentes.

- [ ] Implementar alpha.
  - Verify: sprite semitransparente visible correctamente.

## Phase 5 — Pantallas lógicas

- [ ] Crear superficie lógica superior 256x192.
  - Verify: patrón pixel-perfect renderizado y escalado a 320x240.

- [ ] Crear superficie lógica inferior 256x192.
  - Verify: patrón equivalente en pantalla inferior.

- [ ] Integrar overlay de debug con la superficie inferior.
  - Verify: alternar entre juego y debug sin reiniciar.

## Phase 6 — Datos reales

- [ ] Parsear un contenedor real utilizado por Platinum.
  - Verify: listar su contenido en debug.

- [ ] Cargar un tileset real.
  - Verify: mostrarlo en pantalla.

- [ ] Cargar datos de un mapa real.
  - Verify: mostrar nombre/id/dimensiones correctas.

## Phase 7 — Field

- [ ] Dibujar un mapa real.
  - Verify: mapa reconocible de Platinum visible en emulador.

- [ ] Dibujar protagonista.
  - Verify: sprite del protagonista aparece en la posición correcta.

- [ ] D-Pad mueve al protagonista.
  - Verify: movimiento visible celda a celda.

- [ ] Implementar colisiones.
  - Verify: el jugador no atraviesa una pared conocida.

- [ ] Implementar cámara.
  - Verify: el mapa se desplaza siguiendo al protagonista.

## Phase 8 — Scripts

- [ ] Ejecutar un script mínimo.
  - Verify: aparece un diálogo.

- [ ] Interacción con NPC.
  - Verify: A delante del NPC abre su diálogo.

- [ ] Ejecutar warp.
  - Verify: entrar por una puerta carga otro mapa.

## Phase 9 — Boot real

- [ ] Ejecutar logos.
  - Verify: secuencia visible.

- [ ] Mostrar title screen.
  - Verify: pantalla de título interactiva.

- [ ] New Game.
  - Verify: comenzar la intro.

## Phase 10 — Battle

- [ ] Mostrar battle scene.
  - Verify: fondo y combatientes visibles.

- [ ] Mostrar comandos.
  - Verify: Fight/Bag/Pokémon/Run navegables.

- [ ] Ejecutar un movimiento.
  - Verify: cálculo de daño y HP visible.

- [ ] Terminar combate.
  - Verify: volver correctamente al mapa.

## Phase 11 — Audio

- [ ] Inicializar NDSP.
  - Verify: reproducir tono de prueba.

- [ ] Reproducir un SFX de Platinum.
  - Verify: audible en emulador/hardware.

- [ ] Reproducir música de mapa.
  - Verify: cambia correctamente al cambiar de zona.

## Phase 12 — Save

- [ ] Crear backend de save.
  - Verify: escribir un valor de prueba, reiniciar y recuperarlo.

- [ ] Serializar partida real.
  - Verify: guardar posición del jugador.

- [ ] Cargar partida.
  - Verify: reiniciar y reaparecer en esa misma posición.
```

El roadmap final debe continuar con esta misma filosofía hasta completar el juego base.

---

# Regla de aceptación de cada bullet

Un bullet sólo puede cambiar de:

```text
[ ] -> [x]
```

cuando Codex incluya junto a él:

```text
Verify:
Result:
```

Ejemplo:

```md
- [x] Renderizar primer sprite real.
  - Verify: abrir build `abc123` en Azahar.
  - Result: sprite de Turtwig visible en (128,96), sin corrupción.
```

Si no se puede ejecutar el emulador automáticamente:

```md
- [~] Renderizar primer sprite real.
  - Verify: abrir `.3dsx` y comprobar que Turtwig aparece centrado.
  - Result: pendiente de verificación manual.
```

Es decir:

** no se debe marcar como completada una tarea visual que nadie haya podido verificar.**

---

# Checkpoints ejecutables

Cada milestone importante debe producir un `.3dsx` ejecutable.

Cuando sea práctico, conservar builds identificables o añadir un identificador visible:

```text
Port milestone: FIELD-03
Git: a1b2c3d
```

en la consola de debug.

Esto permite saber exactamente qué build se está probando.

---

# Filosofía de diagnóstico

Cuando un milestone falle, se debe intentar reducir el problema hasta obtener un test visual mínimo.

Ejemplo:

```text
mapa no aparece
↓
¿GPU funciona?
↓
¿puedo mostrar una textura?
↓
¿puedo mostrar este tileset?
↓
¿puedo parsear este mapa?
↓
¿puedo renderizar una única celda?
↓
¿puedo renderizar el mapa completo?
```

No continuar apilando subsistemas encima de uno que no tenga una prueba verificable.

---


# 4. Regla principal para Codex

Codex debe trabajar **incrementalmente**.

NO realizar grandes reescrituras de múltiples subsistemas al mismo tiempo.

Cada milestone debe:

1. compilar;
2. producir un resultado ejecutable cuando sea posible;
3. introducir el mínimo cambio necesario;
4. documentar qué dependencia de Nintendo DS se ha sustituido;
5. añadir pruebas o verificaciones cuando sea razonable;
6. actualizar `roadmap.md`;
7. actualizar `SPECS.md` si cambia una decisión arquitectónica.

Nunca romper deliberadamente el build durante múltiples milestones.

---

# 5. PRIMERA TAREA OBLIGATORIA DE CODEX

Antes de modificar el código del juego, Codex debe crear:

```text
roadmap.md
SPECS.md
```

No comenzar el port hasta que ambos existan.

---

# 6. `roadmap.md`

Codex debe crear inicialmente `roadmap.md`.

Debe ser un roadmap de trabajo **basado principalmente en bulletpoints**.

No debe convertirse en documentación técnica extensa.

Debe servir como checklist de progreso.

**Cada bullet debe incluir un criterio `Verify:` observable o comprobable.**
Siempre que el resultado afecte al runtime, la comprobación preferida será abrir el `.3dsx` en emulador y observar el comportamiento.

Formato recomendado:

```md
# Pokémon Platinum 3DS Port Roadmap

## Phase 0 — Development environment

- [ ] Detect Linux distribution
- [ ] Install host build dependencies
- [ ] Install devkitPro
- [ ] Install 3ds-dev
- [ ] Verify DEVKITPRO
- [ ] Verify DEVKITARM
- [ ] Build hello-world.3dsx

## Phase 1 — Upstream validation

- [ ] Clone pret/pokeplatinum
- [ ] Build original Pokémon Platinum NDS target
- [ ] Verify expected SHA-1
- [ ] Record upstream commit
...
```

Cada tarea debe tener uno de estos estados:

```text
[ ] pendiente
[x] completada
[-] bloqueada
[~] en progreso
```

El roadmap debe modificarse durante todo el proyecto.

No borrar tareas históricas completadas.

---

# 7. `SPECS.md`

Codex debe crear también `SPECS.md`.

Este archivo será la especificación técnica viva del port.

Debe contener como mínimo:

## Project target

- Nintendo 3DS family.
- ARM11 userland.
- `.3dsx` como objetivo inicial.
- C como lenguaje principal.
- C++ únicamente cuando exista una ventaja concreta.
- devkitARM como compilador/toolchain.
- libctru como API de sistema.
- citro2d para render 2D cuando sea adecuado.
- citro3d para GPU/3D cuando sea necesario.
- ndsp para audio.
- romfs para assets empaquetados cuando sea adecuado.

## Supported hardware

Primera fase:

- Old 3DS;
- Old 3DS XL;
- New 3DS;
- New 3DS XL;
- New 2DS XL;
- 2DS.

No depender inicialmente de características exclusivas de New 3DS.

## Screen model

Mantener el modelo lógico original de Nintendo DS:

```text
TOP_LOGICAL_WIDTH  = 256
TOP_LOGICAL_HEIGHT = 192

BOTTOM_LOGICAL_WIDTH  = 256
BOTTOM_LOGICAL_HEIGHT = 192
```

El backend 3DS será responsable de presentar esas superficies lógicas sobre:

```text
3DS top:    400x240
3DS bottom: 320x240
```

No modificar todavía el viewport del juego para mostrar más mundo.

## Coordinate system

Mantener las coordenadas lógicas DS dentro del código de gameplay siempre que sea posible.

La transformación:

```text
DS logical coordinates
        ↓
platform renderer
        ↓
3DS physical coordinates
```

debe pertenecer al backend.

## Input

Crear una representación abstracta.

Ejemplo conceptual:

```c
typedef enum {
    GAME_KEY_A,
    GAME_KEY_B,
    GAME_KEY_X,
    GAME_KEY_Y,
    GAME_KEY_L,
    GAME_KEY_R,
    GAME_KEY_START,
    GAME_KEY_SELECT,
    GAME_KEY_UP,
    GAME_KEY_DOWN,
    GAME_KEY_LEFT,
    GAME_KEY_RIGHT
} GameKey;
```

El gameplay no debe depender directamente de `hidKeysDown()`.

## Filesystem

La lógica del juego no debe conocer rutas `sdmc:/` ni APIs de libctru.

Crear una API de recursos abstracta.

Ejemplo:

```c
bool PlatformFile_Read(
    const char *path,
    void *destination,
    size_t size
);
```

La implementación 3DS puede usar romfs o SD.

## Graphics

Separar conceptualmente:

```text
Game rendering commands
        ↓
Renderer abstraction
        ↓
3DS backend
        ↓
Citro2D / Citro3D
```

No propagar llamadas de Citro2D/Citro3D por todo `src/`.

## Audio

Crear capa independiente.

El juego no debe llamar directamente a NDSP excepto dentro del backend 3DS.

## Time

Crear API para:

- ticks;
- delays;
- frame timing;
- RTC;
- timestamps necesarios por gameplay.

## Save system

Separar serialización de datos de:

- dispositivo físico;
- filesystem;
- rutas;
- flush;
- atomicidad.

## Logging y debug visual

Crear macros o API:

```c
PLAT_LOG_INFO(...)
PLAT_LOG_WARN(...)
PLAT_LOG_ERROR(...)
```

En desarrollo 3DS debe existir una salida razonable de debug.

La implementación debe soportar:

- logging externo cuando esté disponible;
- ring buffer interno;
- consola opcional en la pantalla inferior;
- overlay conmutable durante desarrollo;
- eliminación/desactivación completa del overlay en release.

Configuración conceptual:

```text
DEBUG=1
PORT3DS_DEBUG_OVERLAY=1
```

Durante desarrollo, si no existe una vía fiable para leer logs del emulador, la pantalla inferior será el mecanismo de diagnóstico principal.

El gameplay NO debe depender del overlay.

## Error handling

Los errores fatales no deben producir simplemente un crash silencioso.

Crear una pantalla de error/debug en builds de desarrollo.

## Memory

No asumir que direcciones de memoria, VRAM o regiones de DS existen en 3DS.

Todo acceso hardcoded debe identificarse y clasificarse.

---

# 8. FASE 0 — Preparación del entorno Linux

Esta debe ser la primera fase ejecutada.

Codex debe detectar la distribución Linux antes de instalar paquetes.

Comandos útiles:

```bash
cat /etc/os-release
uname -a
```

## 8.1 Dependencias para compilar `pokeplatinum`

Upstream documenta actualmente estas dependencias.

### Debian / Ubuntu

```bash
sudo dpkg --add-architecture i386
sudo apt update

sudo apt install \
    bison \
    flex \
    g++ \
    gcc-arm-none-eabi \
    git \
    make \
    ninja-build \
    pkg-config \
    wget \
    python3 \
    xz-utils \
    nasm \
    libc6:i386
```

### Arch / derivados

Comprobar primero que `multilib` está habilitado.

Después:

```bash
sudo pacman -S \
    arm-none-eabi-gcc \
    bison \
    flex \
    gcc \
    git \
    make \
    ninja \
    python \
    wget \
    xz \
    lib32-glibc
```

No asumir que los nombres de paquete siguen siendo idénticos.

Si un paquete ha cambiado, Codex debe comprobar el paquete actual equivalente.

---

# 9. Instalación del toolchain Nintendo 3DS

Usar **devkitPro**.

NO intentar construir aplicaciones 3DS utilizando únicamente el `gcc-arm-none-eabi` del sistema.

libctru recomienda devkitARM como toolchain soportado.

Instalar el gestor de paquetes de devkitPro siguiendo su documentación oficial para Linux.

Una vez disponible:

```bash
sudo dkp-pacman -Syu
sudo dkp-pacman -S 3ds-dev
```

Dependiendo del entorno, el binario puede ser `pacman` en lugar de `dkp-pacman`.

Codex debe comprobar cuál está instalado.

Verificar:

```bash
echo "$DEVKITPRO"
echo "$DEVKITARM"
```

Valores normalmente esperables:

```text
DEVKITPRO=/opt/devkitpro
DEVKITARM=/opt/devkitpro/devkitARM
```

No hardcodear esos paths si el instalador proporciona otros.

Verificar:

```bash
$DEVKITARM/bin/arm-none-eabi-gcc --version
```

---

# 10. Hello World 3DS obligatorio

Antes de tocar Pokémon Platinum, Codex debe crear o compilar una aplicación mínima 3DS.

Objetivo:

```text
hello3ds.3dsx
```

Debe:

- inicializar libctru;
- inicializar gráficos;
- escribir texto;
- leer input;
- cerrar al pulsar START.

El milestone sólo se considera completado cuando el `.3dsx` se genera correctamente.

Si existe un entorno de ejecución disponible:

- hardware real;
- emulator compatible;
- CI con validación suficiente;

probar también que arranca.

No bloquear todo el proyecto si sólo falta hardware para probarlo.

---

# 11. FASE 1 — Validación del upstream

Clonar:

```bash
git clone https://github.com/pret/pokeplatinum.git
cd pokeplatinum
```

Registrar:

```bash
git rev-parse HEAD
```

Guardar el commit upstream usado dentro de `SPECS.md`.

Compilar:

```bash
make
```

El proyecto upstream documenta como output principal:

```text
build/pokeplatinum.us.nds
```

Para Rev 1, upstream documenta actualmente:

```text
SHA1
0862ec35b24de5c7e2dcb88c9eea0873110d755c
```

Verificarlo.

No iniciar el port si el upstream original no compila.

El resultado del build original será el **baseline funcional**.

---

# 12. FASE 2 — Auditoría de dependencias de Nintendo DS

Antes de sustituir APIs, construir un inventario.

Crear:

```text
docs/porting/
```

y dentro:

```text
docs/porting/platform-dependencies.md
docs/porting/subsystems.md
docs/porting/nds-api-map.md
```

## `platform-dependencies.md`

Catalogar dependencias en categorías:

- NitroSDK;
- GX;
- G2;
- BG;
- OBJ/OAM;
- VRAM;
- DMA;
- IRQ;
- OS;
- FS;
- CARD;
- RTC;
- touch;
- keypad;
- sound;
- wireless;
- ARM7 IPC;
- overlays;
- memory addresses;
- cache control;
- fixed hardware registers.

## `subsystems.md`

Identificar como mínimo:

- entry point;
- main loop;
- field system;
- map loader;
- map renderer;
- sprite renderer;
- battle system;
- battle renderer;
- UI/window system;
- fonts/text;
- scripting engine;
- input;
- sound;
- filesystem;
- save;
- RNG;
- timing;
- overlays;
- 3D model renderer;
- textures;
- effects;
- communication systems.

Por cada uno indicar:

```text
platform-independent
mostly-independent
mixed
hardware-dependent
unknown
```

## `nds-api-map.md`

Crear una tabla:

```text
NDS API / abstraction
Current users
Purpose
3DS replacement
Status
```

Ejemplo conceptual:

```text
PAD_Read              input       keypad state     PlatformInput
FS_*                  assets      filesystem       PlatformFile
GX_*                  renderer    3D engine        Renderer3DS
G2_*                  renderer    2D engine        Renderer3DS
```

NO hacer sustituciones automáticas por nombre sin entender la semántica.

---

# 13. FASE 3 — Crear el target 3DS sin romper el target NDS

La rama de port debe conservar, cuando sea razonable, la capacidad de construir el juego original.

Objetivos:

```bash
make nds
make 3ds
```

o equivalentes.

No es obligatorio mantener literalmente GNU Make si la arquitectura del repositorio hace más adecuado otro sistema, pero evitar una migración masiva del build system sin necesidad.

El target 3DS inicial debe producir:

```text
build/pokeplatinum-3ds.elf
build/pokeplatinum-3ds.3dsx
```

Aunque inicialmente sólo muestre una pantalla vacía.

---

# 14. FASE 4 — Crear `platform/`

Introducir una capa clara.

Estructura orientativa:

```text
platform/
├── include/
│   └── platform/
│       ├── platform.h
│       ├── input.h
│       ├── graphics.h
│       ├── audio.h
│       ├── filesystem.h
│       ├── save.h
│       ├── time.h
│       ├── memory.h
│       └── debug.h
│
├── nds/
│   └── ...
│
└── 3ds/
    ├── main_3ds.c
    ├── input_3ds.c
    ├── graphics_3ds.c
    ├── audio_3ds.c
    ├── filesystem_3ds.c
    ├── save_3ds.c
    ├── time_3ds.c
    ├── memory_3ds.c
    └── debug_3ds.c
```

Esta estructura es orientativa.

Codex puede adaptarla si descubre que otra separación encaja mejor con el upstream.

Toda desviación importante debe documentarse en `SPECS.md`.

---

# 15. FASE 5 — Entry point y main loop

Objetivo:

Arrancar parte del runtime de Pokémon Platinum dentro de un `.3dsx`.

Primero:

```text
3DS startup
↓
platform init
↓
game init
↓
main loop
↓
platform shutdown
```

El loop 3DS debe respetar:

```c
aptMainLoop()
```

Debe existir una forma controlada de abandonar el juego durante desarrollo.

Inicialmente START puede utilizarse como salida de emergencia cuando no interfiera con el gameplay.

No incorporar todavía lógica de viewport mejorado.

---

# 16. FASE 6 — Timing y frame scheduler

Pokémon Platinum fue diseñado alrededor de las características temporales de Nintendo DS.

Crear una abstracción que permita conservar:

- velocidad lógica;
- timers;
- delays;
- VBlank-dependent code cuando exista;
- animaciones;
- eventos;
- scripts;
- audio timing.

No ligar la lógica directamente al refresco físico de la pantalla 3DS.

Objetivo inicial:

```text
game tick ≈ comportamiento original
```

No intentar 60 FPS.

---

# 17. FASE 7 — Input

Implementar el backend 3DS.

Mapeo inicial conservador:

```text
DS A      -> 3DS A
DS B      -> 3DS B
DS X      -> 3DS X
DS Y      -> 3DS Y
DS L      -> 3DS L
DS R      -> 3DS R
DS START  -> 3DS START
DS SELECT -> 3DS SELECT
D-Pad     -> D-Pad
Touch     -> Touch
```

Circle Pad:

- puede mapearse opcionalmente a D-Pad;
- debe cuantizarse a direcciones digitales;
- NO implementar aún movimiento analógico.

La lógica del juego debe recibir el mismo tipo de estado digital que esperaba originalmente.

---

# 18. FASE 8 — Filesystem y recursos

Determinar cómo `pokeplatinum` construye y consume:

- NARC;
- modelos;
- texturas;
- mapas;
- scripts;
- mensajes;
- sonidos;
- tablas;
- datos generados.

No convertir todos los formatos de assets al principio.

Prioridad:

**reutilizar los formatos originales siempre que el código de parsing sea portable.**

Diseñar el pipeline:

```text
pokeplatinum source assets
        ↓
existing build tools
        ↓
portable generated assets
        ↓
3DS romfs
        ↓
PlatformFile
        ↓
game
```

Evitar que el gameplay lea directamente:

```text
romfs:/...
sdmc:/...
```

---

# 19. FASE 9 — Heap y memoria

Auditar:

- allocators;
- heaps;
- arenas;
- pools;
- offsets;
- memory maps;
- VRAM allocations;
- static buffers;
- alignment;
- cache assumptions.

Separar dos conceptos:

```text
game heap
GPU/resource memory
```

No replicar artificialmente el mapa de memoria de Nintendo DS si no es necesario.

Cuando alguna estructura dependa de tamaños exactos, conservar su layout.

Añadir assertions donde sea útil:

```c
static_assert(...)
```

o equivalente en C.

---

# 20. FASE 10 — Renderer bootstrap

No intentar portar el renderer completo de una vez.

Crear primero el backend capaz de:

1. limpiar pantalla;
2. renderizar una textura;
3. renderizar un sprite;
4. renderizar varios sprites;
5. dibujar primitivas;
6. manejar alpha;
7. manejar palette/texture conversion necesaria;
8. presentar top/bottom screens.

Usar:

- Citro2D para elementos 2D cuando encaje;
- Citro3D para geometría 3D y casos que Citro2D no cubra.

Citro2D ya utiliza Citro3D internamente, por lo que ambos pueden coexistir.

---

# 21. FASE 11 — Modelo lógico de las dos pantallas

Crear dos superficies conceptuales:

```text
GameTopSurface    256x192
GameBottomSurface 256x192
```

Después presentar en la 3DS.

## Pantalla superior

Mantener aspect ratio.

Escalado inicial recomendado:

```text
256x192
   ↓ 1.25x
320x240
```

Centrado dentro de los 400x240 físicos.

Resultado:

```text
+----------------------------------------+
|     |                          |       |
|     |        320 x 240         |       |
|     |                          |       |
+----------------------------------------+
       ^                        ^
       bandas laterales
```

No usar todavía los 400 px como viewport de gameplay.

## Pantalla inferior

El contenido 256x192 puede igualmente escalarse de manera conservadora hacia 320x240.

Esto mantiene el ratio 4:3.

---

# 22. FASE 12 — UI, backgrounds y sprites

Portar progresivamente:

- BG layers;
- tilemaps;
- palettes;
- windows;
- sprites;
- OAM behavior relevante;
- blending;
- fades;
- transitions.

NO es necesario imitar internamente el hardware DS.

Es necesario reproducir el **resultado visible y comportamiento lógico**.

Ejemplo:

```text
DS OAM
   ↓
Sprite abstraction
   ↓
Citro2D
```

---

# 23. FASE 13 — Fuentes y texto

Conservar:

- font data;
- glyph metrics;
- line wrapping;
- message timing;
- text boxes;
- control codes;
- colores;
- velocidad de texto.

No sustituir la tipografía por la fuente de sistema de 3DS en el juego final.

La fuente de sistema sólo puede usarse para debug.

---

# 24. FASE 14 — Field renderer

Objetivo:

Mostrar correctamente un mapa del mundo.

Orden de implementación recomendado:

1. cargar datos de mapa;
2. cargar tiles/modelos;
3. construir escena;
4. dibujar fondo;
5. dibujar geometría;
6. dibujar jugador;
7. dibujar NPCs;
8. ordenar profundidad;
9. animaciones;
10. efectos.

Milestone:

```text
arrancar directamente en un mapa de prueba
```

Sin necesidad todavía de cargar una partida normal.

---

# 25. FASE 15 — Movimiento del jugador

Conseguir:

- spawn;
- D-Pad;
- colisiones;
- facing;
- animación;
- cambio de tile;
- interacción;
- puertas;
- warp.

Milestone:

```text
el jugador puede caminar por un mapa real de Platinum
```

---

# 26. FASE 16 — Scripts y eventos

Verificar que el scripting engine funciona sin depender de:

- timing de DS;
- overlays ausentes;
- callbacks de hardware;
- direcciones absolutas.

Probar:

- hablar con NPC;
- mostrar diálogo;
- choices;
- flags;
- variables;
- give item;
- move NPC;
- warp;
- cutscene sencilla.

---

# 27. FASE 17 — Title screen y boot flow

Una vez las piezas fundamentales funcionen, portar el flujo normal:

```text
startup
↓
logos
↓
title
↓
continue/new game
↓
game
```

Evitar dedicar demasiado esfuerzo a intros antes de validar gameplay.

---

# 28. FASE 18 — Menús

Portar:

- Start menu;
- Bag;
- Pokémon party;
- Pokédex;
- trainer card;
- options;
- summary;
- PC básico;
- shops.

Probar input táctil donde corresponda.

---

# 29. FASE 19 — Sistema de combate

Separar estrictamente:

```text
battle logic
battle presentation
```

La lógica debe reutilizarse tanto como sea posible.

Portar presentación por etapas:

1. battle state initialization;
2. background;
3. battler sprites;
4. HP boxes;
5. text;
6. command UI;
7. animations esenciales;
8. status effects;
9. trainer battles;
10. wild battles;
11. double battles;
12. special battle cases.

Milestone:

```text
wild encounter playable from start to finish
```

Después:

```text
trainer battle playable from start to finish
```

---

# 30. FASE 20 — Audio

Investigar pipeline original antes de reescribir.

Separar:

- music;
- SFX;
- cries;
- sequencing;
- sample playback.

Backend:

```text
Game audio API
      ↓
3DS audio implementation
      ↓
NDSP
```

Si el formato secuenciado original no puede reproducirse razonablemente en NDSP directamente, crear una estrategia de conversión durante build.

No convertir audio manualmente asset por asset.

El pipeline debe ser automatizado.

---

# 31. FASE 21 — Save system

Preservar estructuras de datos originales siempre que sea posible.

Separar:

```text
Save serialization
        ↓
PlatformSave
        ↓
3DS storage
```

Requisitos:

- load;
- save;
- checksum;
- backup/redundancy original cuando proceda;
- recovery;
- flush correcto;
- evitar corrupción si la aplicación se cierra inesperadamente.

Añadir un formato/version marker externo sólo si es estrictamente necesario.

No modificar innecesariamente el formato lógico de guardado.

---

# 32. FASE 22 — Overlays

Pokémon Platinum utiliza overlays como parte de su arquitectura de Nintendo DS.

En 3DS no deben conservarse necesariamente como overlays binarios reales.

Auditar cada overlay y clasificarlo:

```text
code module
data module
hardware-specific
portable gameplay
```

Objetivo:

integrarlos progresivamente como código normal del ejecutable 3DS cuando sea razonable.

No crear un sistema complejo de dynamic linking sólo para imitar DS.

---

# 33. FASE 23 — Código ARM7

Identificar toda funcionalidad que en Nintendo DS dependa de ARM7.

Clasificar:

```text
audio
input/touch
RTC
wireless
system services
other
```

En 3DS:

- no existe la misma división ARM9/ARM7 del software;
- sustituir esas funciones por servicios 3DS;
- eliminar IPC específico de DS cuando ya no tenga sentido.

No intentar emular ARM7.

---

# 34. FASE 24 — Funciones online y comunicación

Para el primer port jugable:

- no es necesario reimplementar Nintendo Wi-Fi Connection;
- no bloquear el juego por ausencia de networking;
- desactivar limpiamente funciones imposibles;
- evitar crashes al entrar en menús relacionados.

Las funciones locales/multijugador pueden posponerse.

Documentar explícitamente qué funcionalidades quedan deshabilitadas.

---

# 35. FASE 25 — Integración del juego completo

Una vez funcionen los subsistemas:

Probar una partida normal desde:

```text
New Game
```

y recorrer progresivamente:

- intro;
- starter;
- primeros combates;
- menús;
- rutas;
- ciudades;
- gimnasios;
- cuevas;
- HM;
- encounters;
- evoluciones;
- tiendas;
- Pokémon Center;
- PC;
- eventos;
- cutscenes;
- Elite Four;
- postgame básico.

Crear una matriz de compatibilidad.

Archivo:

```text
docs/porting/compatibility.md
```

Ejemplo:

```md
| Feature | Status | Notes |
|---|---|---|
| Boot | Working | |
| New Game | Working | |
| Save | Working | |
| Wild battles | Working | |
| Trainer battles | Partial | Animation issue |
```

---

# 36. FASE 26 — Testing y regresiones

Mantener al menos tres tipos de pruebas.

## Host tests

Para código independiente de hardware:

- parsers;
- serializers;
- battle calculations;
- data structures;
- utility functions.

## 3DS smoke tests

- boot;
- filesystem;
- rendering;
- input;
- audio;
- save.

## Gameplay tests

Checkpoints reproducibles de partida.

Cuando sea posible crear herramientas para:

- teleport;
- cargar mapa concreto;
- iniciar battle concreto;
- dar Pokémon;
- cambiar flags;
- ejecutar script.

Estas herramientas sólo estarán activas en builds de debug.

---

# 37. FASE 27 — Debug tooling

Crear build:

```text
DEBUG=1
```

Debe ofrecer cuando sea razonable:

- logging;
- FPS;
- frame time;
- heap usage;
- GPU time;
- current map;
- player coordinates;
- active script;
- current overlay/module;
- assert screen;
- crash diagnostics.

Evitar ensuciar el gameplay release.

---

# 38. FASE 28 — Rendimiento

Optimizar sólo después de tener funcionalidad.

Medir:

- CPU;
- GPU;
- command buffer;
- memory;
- texture uploads;
- draw calls;
- asset loading;
- audio.

No hacer optimizaciones especulativas.

Prioridad:

```text
correctness
↓
stability
↓
performance
```

Objetivo de esta etapa:

mantener la velocidad equivalente al juego original de forma estable en Old 3DS.

---

# 39. FASE 29 — Release base

El primer release funcional debe producir:

```text
pokeplatinum-3ds.3dsx
```

y los recursos necesarios.

Debe incluir documentación:

```text
README-3DS.md
BUILDING-3DS.md
KNOWN-ISSUES.md
```

`BUILDING-3DS.md` debe permitir a un desarrollador empezar desde una instalación Linux limpia.

---

# 40. Definition of Done de esta etapa

Esta etapa termina cuando se cumplen todos estos puntos:

- [ ] El proyecto compila en Linux.
- [ ] El upstream NDS sigue teniendo un baseline verificable.
- [ ] Existe target nativo 3DS.
- [ ] Se genera `.3dsx`.
- [ ] Arranca en hardware/emulador compatible.
- [ ] Input funcional.
- [ ] Touch funcional donde sea necesario.
- [ ] Pantalla superior funcional.
- [ ] Pantalla inferior funcional.
- [ ] Mapas funcionales.
- [ ] Sprites funcionales.
- [ ] Modelos 3D esenciales funcionales.
- [ ] Scripts funcionales.
- [ ] Eventos funcionales.
- [ ] Menús principales funcionales.
- [ ] Combates salvajes funcionales.
- [ ] Combates de entrenadores funcionales.
- [ ] Audio funcional.
- [ ] Guardado funcional.
- [ ] Carga funcional.
- [ ] Warps funcionales.
- [ ] Pokémon Center funcional.
- [ ] Tiendas funcionales.
- [ ] PC funcional.
- [ ] Progresión normal del juego funcional.
- [ ] El juego mantiene velocidad correcta en Old 3DS.
- [ ] Existe build DEBUG con consola inferior conmutable.
- [ ] Existe build RELEASE sin dependencia del overlay de debug.
- [ ] Cada milestone funcional del roadmap tiene un criterio de verificación reproducible.
- [ ] Las funciones no soportadas fallan limpiamente.
- [ ] No existen crashes conocidos que bloqueen una partida normal.

---

# 41. STOP POINT

Cuando se alcance el Definition of Done anterior:

**DETENER EL DESARROLLO DE NUEVAS CARACTERÍSTICAS.**

No iniciar automáticamente una segunda etapa.

En concreto, NO comenzar:

```text
400x240 gameplay viewport
widescreen
stereoscopic 3D
Circle Pad analog movement
C-Stick
higher resolution assets
graphics enhancements
draw-distance improvements
60 FPS
New 3DS enhancements
```

Esos elementos pertenecen a una futura:

```text
PHASE 2 — 3DS Enhancements
```

y requieren una especificación independiente.

---

# 42. Estrategia de commits para Codex

Hacer commits pequeños y conceptuales.

Ejemplos:

```text
build: add native 3DS target
platform: add 3DS bootstrap
platform: abstract input backend
platform: implement 3DS HID input
fs: add platform filesystem abstraction
renderer: add 3DS render target initialization
renderer: implement sprite drawing
field: render first map on 3DS
save: implement 3DS save backend
audio: initialize NDSP backend
```

Evitar commits:

```text
port lots of stuff
3ds fixes
working now
misc changes
```

---

# 43. Política respecto a upstream

Mantener los cambios del port lo menos invasivos posible.

Preferir:

```c
Platform_InputRead(...)
```

a:

```c
#ifdef __3DS__
hidScanInput();
...
#else
...
#endif
```

repetido por cientos de archivos.

Los `#ifdef __3DS__` deben estar concentrados principalmente en:

```text
platform/
build configuration
small integration boundaries
```

No convertir el proyecto en una red de conditional compilation.

---

# 44. Regla para problemas desconocidos

Cuando Codex encuentre una función no entendida:

1. localizar todas sus referencias;
2. identificar el subsistema;
3. estudiar su implementación NDS;
4. determinar el comportamiento observable;
5. documentarlo;
6. implementar la abstracción mínima;
7. probarla.

NO eliminar código simplemente porque no compile.

NO sustituir funciones desconocidas por stubs permanentes que devuelvan `0`.

Un stub temporal debe estar marcado:

```c
// TODO(PORT3DS): temporary stub
```

y registrado en:

```text
docs/porting/stubs.md
```

---

# 45. Tracking de stubs

Crear:

```text
docs/porting/stubs.md
```

Formato:

```md
| Function | Subsystem | Current behavior | Required behavior | Blocking |
|---|---|---|---|---|
```

El Definition of Done no permite stubs bloqueantes para gameplay normal.

---

# 46. Build reproducible

Registrar versiones relevantes:

```text
pokeplatinum upstream commit
devkitARM version
libctru version
citro2d version
citro3d version
host distro
host compiler
Python version
```

No fijar versiones antiguas salvo que exista una incompatibilidad demostrada.

Preferir mantenerse compatible con versiones actuales de devkitPro.

---

# 47. Seguridad de los datos originales

No incluir ROMs comerciales en el repositorio.

No descargar automáticamente ROMs.

No añadir assets cuya redistribución no corresponda al proyecto upstream.

El proceso debe partir del material que el proyecto de decompilación ya espera o genera legalmente dentro de su flujo normal.

---

# 48. Primera secuencia concreta de trabajo para Codex

Al recibir este documento, ejecutar en este orden:

```text
1. Inspeccionar repositorio.
2. Leer README.md.
3. Leer INSTALL.md.
4. Leer CONTRIBUTING.md.
5. Inspeccionar Makefile y meson.build.
6. Inspeccionar src/, include/, lib/, res/, subprojects/ y tools/.
7. Crear roadmap.md.
8. Crear SPECS.md.
9. Detectar distribución Linux.
10. Instalar dependencias host.
11. Compilar pokeplatinum original.
12. Verificar SHA-1.
13. Instalar/configurar devkitPro.
14. Instalar 3ds-dev.
15. Compilar hello-world 3DS.
16. Implementar Debug API.
17. Implementar ring buffer y consola de debug inferior.
18. Implementar toggle del overlay y build sin overlay.
19. Crear documentación de auditoría.
20. Identificar entry point y main loop.
18. Crear target 3DS mínimo.
19. Crear platform abstraction.
20. Arrancar .3dsx vacío.
21. Migrar timing.
22. Migrar input.
23. Migrar filesystem.
24. Migrar memoria.
25. Crear renderer básico.
26. Mostrar primera textura/sprite.
27. Implementar dos pantallas lógicas.
28. Portar UI básica.
29. Portar field renderer.
30. Mostrar primer mapa.
31. Habilitar movimiento.
32. Habilitar scripts.
33. Habilitar eventos.
34. Restaurar boot flow.
35. Portar menús.
36. Portar combate.
37. Portar audio.
38. Portar save/load.
39. Resolver overlays.
40. Resolver dependencias ARM7.
41. Deshabilitar limpiamente networking no soportado.
42. Ejecutar recorrido completo de compatibilidad.
43. Optimizar sólo los cuellos de botella medidos.
44. Alcanzar Definition of Done.
45. DETENERSE antes de las mejoras específicas de 3DS.
```

---

# 49. Resultado esperado

La arquitectura final de esta primera etapa debería parecerse conceptualmente a:

```text
pokeplatinum/
│
├── src/
│   └── gameplay portable
│
├── include/
│
├── res/
│
├── tools/
│
├── platform/
│   ├── include/
│   ├── nds/
│   └── 3ds/
│
├── docs/
│   └── porting/
│
├── roadmap.md
├── SPECS.md
├── BUILDING-3DS.md
├── README-3DS.md
└── KNOWN-ISSUES.md
```

El port 3DS debe ser un **backend de plataforma**, no una reescritura del juego.

La meta es llegar primero a:

> Pokémon Platinum completo, funcional y estable ejecutándose nativamente en Nintendo 3DS.

Sólo después deberá diseñarse una segunda fase para transformar ese port conservador en una versión que aproveche específicamente el hardware adicional de Nintendo 3DS.
