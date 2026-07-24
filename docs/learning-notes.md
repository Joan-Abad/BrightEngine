# Notas de aprendizaje — BrightEngine

Este documento recoge, en el orden en que se han ido explicando, los
conceptos de desarrollo de motores que hemos cubierto trabajando en
BrightEngine. No es documentación de referencia para agentes ni
convenciones técnicas del repo (para eso están los docs bajo
`engine/docs/`) — es un cuaderno personal para repasar el *por qué* de
cada pieza, no solo el *qué*. El objetivo del proyecto en sí es aprender
el proceso de construir un motor; el motor es la excusa.

Se actualiza según avanzamos, tema a tema.

---

## Rendering: cómo llega un píxel a la pantalla

**El mapa general:**

```text
Tu código (game/app)
      │
      ▼
   RHI (la abstracción que estamos diseñando)
      │
      ▼
Driver de la GPU (OpenGL/Vulkan/D3D12/Metal)
      │
      ▼
GPU física
```

La RHI existe porque OpenGL, Vulkan, D3D12 y Metal son APIs completamente
distintas entre sí, pero resuelven el mismo problema de fondo. Por eso
casi cualquier motor real converge en el mismo puñado de conceptos
abstractos, sea cual sea la API real por debajo:

1. **Device** — la conexión con la GPU; crea todo lo demás.
2. **Recursos** (buffers, texturas) — datos en memoria de GPU: vértices,
   índices, texturas, uniforms/constantes.
3. **Pipeline state** — configuración fija de cómo se dibuja algo (qué
   shaders, cómo interpretar vértices, blending, profundidad...). Se
   define una vez, se reutiliza muchas veces.
4. **Command list/buffer** — lista de órdenes grabadas ("dibuja esto con
   este pipeline") que se envían a la GPU en bloque, en vez de ejecutar
   cada llamada de inmediato.
5. **Queue/submit + sincronización** — cómo le dices a la GPU "ejecuta
   esto ya" y cómo sabes cuándo ha terminado, para no pisar datos que
   todavía está usando.

**La ventana no sabe nada de renderizado por sí sola.** `glfwCreateWindow`
solo crea un rectángulo gestionado por el sistema operativo — no tiene
ninguna conexión con la GPU hasta que se le asocia un **contexto de
renderizado**.

**El contexto** es, en OpenGL, un objeto gigante con estado: qué buffer
está activo, qué shader está activo, qué hay en pantalla... todo vive
ahí. `glfwMakeContextCurrent(window)` es lo que le dice a un hilo "a
partir de ahora, las órdenes de OpenGL van dirigidas a este contexto".
Con GLFW, cada ventana se lleva su propio contexto por defecto — pero
contexto y ventana son conceptualmente cosas distintas (se puede incluso
compartir recursos, como texturas, entre contextos de ventanas distintas).

**La regla de threading exacta no es "un hilo por ventana"** — es más
específica: **un contexto dado solo puede estar "activo" (current) en un
único hilo a la vez.** Dos hilos SÍ pueden llevar, cada uno, su propia
ventana/contexto en paralelo sin problema (son objetos de contexto
distintos); lo que no puedes hacer es tener el **mismo** contexto activo
en dos hilos simultáneamente (aunque sí puedes moverlo de un hilo a otro
en momentos distintos). La razón: el contexto es un bloque enorme de
estado global mutable y las llamadas de OpenGL son funciones globales que
operan implícitamente sobre "el contexto activo ahora" — sin esta regla,
dos hilos tocándolo a la vez serían una condición de carrera garantizada.
Esto es precisamente parte de lo que motivó que Vulkan/D3D12/Metal
permitan grabación de comandos genuinamente multi-hilo (cada hilo con su
propio command buffer, sincronización explícita) — ver la sección
siguiente.

**El framebuffer** es una colección de imágenes en memoria de GPU (color,
y opcionalmente profundidad) sobre las que las órdenes de dibujo
escriben. Existe un framebuffer "por defecto" conectado a la ventana, pero
se pueden crear framebuffers propios fuera de pantalla (sombras,
post-procesado, reflejos...).

**El swapchain** — si la GPU dibujara directamente sobre la imagen que el
monitor muestra en ese instante, se vería el frame a medio dibujar
(tearing). La solución: al menos dos imágenes, una mostrándose ahora
("front buffer") y otra en la que se dibuja el frame siguiente ("back
buffer"). Al terminar un frame no se copia nada, se intercambian los
roles — eso es `glfwSwapBuffers(window)`.

OpenGL oculta casi todo este mecanismo (solo llamas `glfwSwapBuffers` y
ya). En Vulkan/D3D12/Metal, el swapchain es un objeto explícito que tú
gestionas a mano (pedir la siguiente imagen libre, avisar cuando la GPU
termina con una imagen antes de reutilizarla...). Esta diferencia —
implícito en OpenGL, explícito en las APIs modernas — es la razón
concreta por la que la RHI necesita su propia abstracción del concepto de
swapchain.

Detalle adicional: `glfwSwapInterval(1)` controla si el intercambio espera
la sincronización vertical del monitor (V-Sync). Sin esperar: posible
tearing pero menor latencia. Esperando: más suave pero limitado a los Hz
del monitor.

**Resumen del flujo completo:** ventana → contexto → framebuffer →
swapchain → present.

**¿Y GLFW, de dónde sale?** No tiene nada que ver con Khronos. Khronos
define las especificaciones de OpenGL/Vulkan, pero deliberadamente no
cubre "cómo crear una ventana" — eso depende del sistema operativo (Win32,
X11/Wayland, Cocoa), y las APIs de gráficos solo se ocupan de qué pasa una
vez que ya existe una superficie válida donde dibujar. Ese hueco lo cubren
APIs de plataforma (WGL, GLX, EGL) o librerías de terceros como GLFW, que
envuelven esas diferencias en una sola API consistente. GLFW es un
proyecto independiente (licencia zlib/libpng), y hace deliberadamente solo
tres cosas: crear la ventana, crear el contexto/superficie asociado, y
gestionar input + el bucle de eventos — nada de audio, networking ni UI
(para eso existen alternativas más amplias como SDL2 o Qt).

**Siguiente escalón:** cómo decirle a la GPU *qué* dibujar dentro de ese
framebuffer — buffers de vértices y shaders, hasta llegar al primer
triángulo real. Ver la siguiente sección.

---

## De los vértices al primer triángulo

**Un vértice es un paquete de datos**, no solo un punto: como mínimo una
posición (x, y, z), y normalmente más "atributos" pegados (color,
coordenadas de textura/UV, normal para iluminación...).

**El buffer** — la GPU no puede leer eficientemente la RAM de la CPU en el
camino crítico de dibujado, así que los datos de vértices se suben a
memoria de GPU en un bloque llamado **Vertex Buffer** (VBO en OpenGL). Una
vez subido, dibujar ya no vuelve a tocar la RAM de la CPU.

**El layout** — un buffer es, para la GPU, solo una secuencia de bytes sin
significado propio. Si mezclas posición + color en el mismo array, hay
que describirle explícitamente a la GPU qué floats son posición, cuáles
son color, y cuánto ocupa cada vértice (el *stride*). En OpenGL esa
descripción vive en un **Vertex Array Object (VAO)**, que recuerda la
asociación buffer↔layout para no repetirla en cada draw call.

**Shaders — programas que corren en la GPU, no en la CPU.** Antes de
~2004 esta parte era "fixed-function" (opciones fijas, sin programar).
Hoy es programable: se ejecutan masivamente en paralelo, una instancia
por vértice o por píxel. Mínimo hacen falta dos:

- **Vertex shader** — una vez por vértice. Recibe sus atributos, devuelve
  su posición final (clip-space). Puede pasar datos extra al siguiente
  shader (ej. color), que la GPU interpola automáticamente entre vértices.
- **Fragment shader** ("pixel shader" en D3D) — una vez por cada píxel
  que el triángulo cubre en pantalla. Devuelve el color final de ese
  píxel.

Entre uno y otro, la GPU **rasteriza** automáticamente (calcula qué
píxeles caen dentro del triángulo) e interpola los valores del vertex
shader sobre la superficie — así un triángulo con 3 vértices de colores
distintos sale con degradado suave sin que se calcule a mano.

**El pipeline** — el conjunto de decisiones (qué vertex shader, qué
fragment shader, qué layout, estados fijos como blending/profundidad)
empaquetado. En OpenGL "clásico" esto está disperso e implícito (bind
shader aquí, bind VAO allá, algún `glEnable` suelto); en Vulkan/D3D12 es
un único objeto inmutable creado de una vez ("Pipeline State Object" /
PSO) — agrupar todo permite validar y optimizar una sola vez en vez de
revalidar en cada draw call. Esto es lo que la RHI llama `IPipeline`.

**El draw call** — con el buffer subido, el layout descrito y el pipeline
definido, una única llamada (`glDrawArrays`/`glDrawElements`) dispara todo
el proceso: ejecutar el vertex shader por vértice, ensamblar triángulos,
rasterizar, ejecutar el fragment shader por píxel, escribir en el
framebuffer.

**Camino completo:** vértices → buffer → layout → shaders → pipeline →
draw call → framebuffer → swapchain → pantalla.

---

## Qué son realmente OpenGL, Direct3D, Vulkan y Metal

No son un envoltorio fino que "llama a funciones nativas de la GPU" — hay
una pieza intermedia enorme: el **driver**.

```text
Tu código
    │  llama a la API (OpenGL / Direct3D / Vulkan / Metal)
    ▼
El DRIVER de la GPU  ← software escrito por NVIDIA/AMD/Intel/Apple/Qualcomm
    │  traduce, valida, a veces compila shaders, optimiza
    ▼
Comandos en el lenguaje máquina específico de ESA GPU
    │
    ▼
GPU física
```

El driver hace trabajo real y pesado: valida las llamadas, decide cómo
organizar la memoria de GPU, a menudo compila los shaders (escritos en un
lenguaje de alto nivel) al lenguaje máquina real de esa GPU concreta, y
toma decisiones de optimización que el programador no ve. Por eso
"OpenGL" no es una sola pieza de código universal — es una
**especificación**, y cada fabricante la implementa en su propio driver a
su manera. Eso explica por qué el "mismo" juego se comporta distinto según
la marca de GPU: cada driver interpreta/optimiza la especificación de
forma distinta.

**Quién mantiene cada API y en qué plataformas corre:**

- **OpenGL** — mantenida por Khronos Group (consorcio: NVIDIA, AMD,
  Intel...). Plataformas: Windows, Linux, macOS (deprecada), Android
  (vía OpenGL ES).
- **Vulkan** — mantenida por Khronos Group (sucesor moderno de OpenGL).
  Plataformas: Windows, Linux, Android, macOS/iOS vía capa de traducción
  (MoltenVK).
- **Direct3D (DirectX)** — propietaria de Microsoft. Plataformas: solo
  Windows/Xbox.
- **Metal** — propietaria de Apple. Plataformas: solo macOS/iOS.

**Los drivers, un poco más de detalle:**

- Suelen tener dos partes: un componente en **modo kernel** (habla
  directamente con el hardware, gestiona memoria de GPU y planificación —
  normalmente cerrado y específico del chip) y un componente en **modo
  usuario** (implementa la API en sí — en Windows, `opengl32.dll` no es
  "OpenGL", es una capa que reenvía al driver instalado de la GPU).
- Un mismo driver de un fabricante suele implementar varias APIs a la
  vez (el driver de NVIDIA en Windows trae dentro su OpenGL, su Vulkan y
  su Direct3D simultáneamente, no son drivers separados por API).
- Por eso actualizar el driver no es cosmético: cambia cómo se comporta
  OpenGL/Vulkan por debajo del código, arregla bugs de esa implementación,
  e incluso mete parches/optimizaciones específicas para juegos concretos
  detectados por el nombre del ejecutable.
- En Linux existe además **Mesa**, un proyecto open-source que implementa
  OpenGL/Vulkan para AMD e Intel, con un modo de software puro por CPU sin
  GPU real (`llvmpipe`) — relevante en máquinas virtuales o contenedores
  sin GPU dedicada.

**"Viejas" (OpenGL, D3D11) vs. "modernas" (Vulkan, D3D12, Metal) — no es
solo edad, es un cambio de filosofía:**

- OpenGL/D3D11: el driver gestiona memoria, sincronización y orden de
  comandos de forma implícita. Más fácil de programar, menos control, y
  esa "magia" implícita es justo la que varía entre fabricantes.
- Vulkan/D3D12/Metal: el programador gestiona memoria y sincronización
  (barriers/fences) explícitamente, y puede grabar comandos desde varios
  hilos a la vez. Más verboso y fácil de hacer mal, pero rendimiento más
  predecible y menos overhead del driver por llamada.

Esto es la misma razón, aplicada en general, por la que el swapchain es
implícito en OpenGL pero explícito en las APIs modernas (ver sección
anterior).

**Detalle para más adelante:** cada API tiene su propio lenguaje de
shaders — GLSL (OpenGL/Vulkan), HLSL (Direct3D), MSL (Metal). Cuando la
RHI soporte varios backends, esto será un problema real a resolver
(no ahora).
