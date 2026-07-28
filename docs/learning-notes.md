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

**Un shader no es solo una abstracción de la API — es la arquitectura
física del chip.** Antes de la era programable (~2001-2006), las GPUs
tenían circuitos fijos y dedicados por tarea (una unidad de
"transformación e iluminación" cableada, un combinador de texturas con
parámetros ajustables pero no programables). La transición a shaders
programables sustituyó esos circuitos especializados por arrays enormes
de núcleos pequeños y genéricos (NVIDIA: "CUDA cores"; AMD: "stream
processors"; Intel: "execution units"). Desde la **arquitectura de shader
unificada** (~2006-2007, GeForce 8800/Xbox 360), el mismo núcleo físico
ejecuta indistintamente código de vertex shader, fragment shader o
compute shader — no hay ya hardware separado por etapa, es el mismo
silicio genérico alimentado con distinto programa y distintos datos. Por
eso escribir un shader es programar literalmente lo que se carga en esos
núcleos, no una capa de software desconectada del chip.

**Cómo se ejecutan miles de instancias a la vez:** las GPUs agrupan
instancias del shader en bloques (NVIDIA: "warps", 32 hilos; AMD:
"wavefronts", 64 hilos) donde **todos los hilos del grupo ejecutan la
misma instrucción a la vez, sobre datos distintos** (SIMT — Single
Instruction, Multiple Threads). Es la razón de que las GPUs sean
extremadamente rápidas para aplicar la misma operación a millones de
píxeles, pero comparativamente malas con código muy ramificado: si dentro
del mismo grupo unos hilos toman un camino de un `if` y otros el `else`,
el hardware ejecuta ambos caminos en serie para todo el grupo
("divergencia de rama") — una fuente real de pérdida de rendimiento en
shaders con lógica condicional compleja.

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

**Detalle práctico importante: los errores de GLSL son invisibles si no
los compruebas.** El código de un shader es, para el compilador de C++,
solo un string — un error de sintaxis en el GLSL no da ningún error de
compilación de C++. Sin comprobar explícitamente el resultado
(`glGetShaderiv(..., GL_COMPILE_STATUS, ...)` tras `glCompileShader`, y
`glGetProgramiv(..., GL_LINK_STATUS, ...)` tras `glLinkProgram`, leyendo
el mensaje real con `glGetShaderInfoLog`/`glGetProgramInfoLog` si falla),
un shader roto simplemente no dibuja nada (o dibuja basura) sin ningún
aviso — el fallo es completamente silencioso. Por eso se comprueba
siempre, no es opcional ni paranoia de más.

**Ya se ha construido un primer triángulo real** en `sandbox/main.cpp`,
con OpenGL "a pelo" (sin ninguna capa RHI todavía) — sirvió para sentir
en carne propia cada pieza de este camino antes de diseñar la
abstracción. Ese código es intencionalmente temporal: se espera que gran
parte de esta lógica se mueva a `engine/src/rhi/opengl/` una vez diseñada
la RHI.

---

## Uniforms — pasar datos desde la CPU sin recompilar el shader

Un **uniform** es un valor que permanece **constante para todos los
vértices y todos los píxeles dentro de un mismo draw call** — de ahí el
nombre. Es el contrapunto directo de un atributo: el atributo varía
vértice a vértice y viene del buffer; el uniform es un único valor que
subes desde la CPU justo antes de dibujar, y puede cambiar de un frame a
otro sin tocar el buffer de vértices ni recompilar nada.

Se declara en GLSL con la palabra clave `uniform` (`uniform float
uRotation;`), y desde C++ se referencia por nombre:
`glGetUniformLocation(shaderProgram, "uRotation")` (una vez, tras enlazar
el program; devuelve `-1` si el nombre no existe o el compilador lo
optimizó por no usarse), y se actualiza cada frame con
`glUniform1f(location, valor)` — siempre **después** de `glUseProgram`,
porque un uniform solo afecta al program actualmente activo (mismo patrón
de estado global de siempre).

No se limita a floats sueltos — puede ser `vec3`, `vec4`, y sobre todo
**matrices** (`mat4`, para transformaciones de modelo/vista/proyección
más adelante con GLM). Incluso las **texturas** se referencian mediante
un uniform (`sampler2D`, que por debajo es solo un entero indicando qué
unidad de textura usar). Lo típico que se pasa como uniform en un motor
real: matrices de transformación, cámara, luces, tiempo, propiedades de
material, resolución de pantalla — cualquier "parámetro" del draw call
que no sea geometría en sí.

**Para más adelante:** subir uniforms uno a uno con `glUniform1f` (etc.)
funciona bien con pocos valores, pero motores reales que suben muchos
datos relacionados a la vez (una matriz completa + varias luces) suelen
usar **Uniform Buffer Objects (UBO)** — agrupan varios uniforms en un
solo buffer de GPU, actualizable de una vez y compartible entre distintos
shader programs.

---

## Las matemáticas de una rotación 2D, paso a paso

**Un vector es solo una lista de números** — `(x, y)` es una flecha desde
el origen `(0,0)` hasta ese punto.

**Qué calcula "matriz por vector":**

```text
[a  b]   [x]   [a·x + b·y]
[c  d] · [y] = [c·x + d·y]
```

**La idea clave — una matriz son "adónde van los vectores base".**
Cualquier punto `(x,y)` se puede escribir como `x·(1,0) + y·(0,1)`. Si
sabes adónde manda una transformación a `(1,0)` y a `(0,1)`, sabes adónde
manda cualquier punto. Las **columnas** de una matriz son, literalmente,
adónde aterrizan esos dos vectores base tras la transformación.

**Derivando la matriz de rotación:** rotar `(1,0)` un ángulo `θ` lo manda
a `(cos θ, sin θ)` (trigonometría básica). La columna de `(0,1)` sale de
un razonamiento distinto (ver más abajo), dando:

```text
R(θ) = [cos θ   -sin θ]
       [sin θ    cos θ]
```

Multiplicar `R(θ)` por `(x,y)` da `x' = x·cos θ − y·sin θ` y también
`y' = x·sin θ + y·cos θ` — exactamente la fórmula manual usada antes de
introducir GLM. No era una fórmula aparte: era esta matriz, escrita
componente a componente.

**Por qué la columna de `(0,1)` lleva `-sin`, no `sin`:** `(0,1)` está
siempre exactamente 90° por delante de `(1,0)` en el círculo, y rotar
preserva esa separación de ángulo. Así que, sea cual sea el ángulo `θ`
que rotes, `(0,1)` siempre acaba 90° por delante de donde acabó `(1,0)`.
Rotar cualquier punto `(a,b)` otros 90° más da siempre `(-b, a)`
(intercambiar coordenadas, negar la nueva primera — un caso particular de
la misma fórmula general, evaluada en `θ=90°`). Aplicando esa regla al
punto donde aterrizó `(1,0)`, que es `(cos θ, sin θ)`: `(-sin θ, cos θ)`
— exactamente la segunda columna. No es una fórmula distinta para
`(0,1)`, es la misma regla de rotación aplicada a un vector que ya
empieza 90° adelantado.

**El problema de la traslación:** ninguna matriz puede mover el origen
`(0,0)` — multiplicar cualquier matriz por `(0,0)` siempre da `(0,0)`.
Como una traslación mueve el origen por definición, no puede
representarse como una matriz normal.

**La solución — coordenadas homogéneas:** añadir una coordenada extra
`w=1` a cada punto (`(x,y)` → `(x,y,1)`) permite construir una matriz que
sí traslada, colando el desplazamiento en una multiplicación lineal sobre
esa componente extra:

```text
[1  0  tx]   [x]   [x + tx]
[0  1  ty] · [y] = [y + ty]
[0  0   1]   [1]   [  1   ]
```

**De 2D a 3D — por qué `mat4`:** mismo truco, una dimensión más: un punto
3D se convierte en `(x,y,z,w=1)`, la matriz pasa a ser 4x4, con la
rotación/escala en el bloque 3x3 superior-izquierdo y la traslación en la
última columna. Por eso `gl_Position` es `vec4`, y por eso todo en
gráficos usa `mat4` — es la forma mínima de meter traslación + rotación +
escala en una sola multiplicación.

**Multiplicar matrices = encadenar transformaciones.** El resultado de
multiplicar dos matrices de transformación aplica ambas en secuencia —
por eso `glm::rotate(glm::mat4(1.0f), ángulo, eje)` funciona: parte de la
identidad (matriz "no hagas nada") y la multiplica por la rotación.

---

## Index buffers (EBO) — reutilizar vértices compartidos

Cuando varios triángulos comparten un vértice (ej. un cuadrado hecho de 2
triángulos comparte 2 esquinas en la diagonal), un **Element Buffer
Object** evita duplicar los datos completos de ese vértice: el buffer de
vértices guarda solo los vértices **únicos**, y un buffer aparte de
**índices** (enteros que apuntan a esos vértices) describe qué vértice
usa cada triángulo. Se crea igual que un VBO pero con el target
`GL_ELEMENT_ARRAY_BUFFER` en vez de `GL_ARRAY_BUFFER`, y su binding queda
grabado dentro del VAO activo (no hace falta re-vincularlo cada frame). El
draw call correspondiente es `glDrawElements` en vez de `glDrawArrays`.

**Por qué importa, en números:** un índice (`unsigned int`) son 4 bytes.
Un vértice completo (posición + color, en nuestro caso) son 6 floats ×
4 bytes = 24 bytes — reutilizar vía índice cuesta una sexta parte de
duplicar el vértice entero. En mallas reales, donde un vértice interior
suele compartirse entre ~6 triángulos y cada vértice lleva posición +
normal + UV + tangente + color (40-50+ bytes), el ahorro de memoria es
mucho mayor que en un simple cuadrado.

**El ahorro más importante no es de memoria, es de cómputo.** Las GPUs
tienen una caché pequeña llamada **post-transform vertex cache**: si el
mismo índice vuelve a aparecer poco después en el buffer de índices (lo
normal en una malla bien ordenada con vértices compartidos), la GPU
reutiliza el resultado ya calculado del vertex shader para ese vértice en
vez de volver a ejecutarlo. Los índices no solo ahorran ancho de banda —
evitan recalcular el mismo vértice transformado varias veces, lo cual
importa más cuando el vertex shader es caro (skinning de personajes,
morph targets).

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

---

## Texturas y unidades de textura

Una textura se sube a la GPU con un patrón similar a un buffer de
vértices, pero con su propio objeto (`glGenTextures`/`glBindTexture`) y
su propia función de subida, `glTexImage2D` (equivalente a `glBufferData`
pero para imágenes). Se muestrea en el fragment shader mediante un
`uniform sampler2D` y la función `texture(sampler, uv)`, usando
coordenadas UV (`0.0`–`1.0` por eje) que llegan como un atributo más,
interpoladas igual que el color.

Parámetros clave al crear una textura:

- `GL_TEXTURE_WRAP_S/T` — qué hacer si una UV cae fuera de `[0,1]`
  (`GL_REPEAT` la repite en mosaico).
- `GL_TEXTURE_MIN/MAG_FILTER` — cómo interpolar entre texels al reducir
  (`MIN`) o ampliar (`MAG`) en pantalla. `GL_NEAREST` = sin suavizar
  (bloques nítidos); `GL_LINEAR` = mezcla los texels vecinos.

**Unidades de textura — dos capas distintas, no confundir:**

- **TMU (Texture Mapping Units)** — hardware físico real: circuitos
  dedicados en el chip que calculan direcciones de texel, aplican
  filtrado, leen la caché de textura. Su número es una característica
  fija de cada modelo de GPU concreto.
- **"Unidad de textura" de la API** (`GL_TEXTURE0`, `GL_TEXTURE1`...,
  seleccionadas con `glActiveTexture`) — una **abstracción lógica**, no
  un TMU físico dedicado cada una. Es una referencia que le dice al
  driver "esta textura enlazada corresponde a este número"; el driver
  reparte el trabajo real de muestreo entre las TMUs físicas disponibles
  sin que exista una asignación 1:1 unidad↔TMU. El número disponible se
  consulta en tiempo de ejecución (`GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS`;
  el spec de OpenGL exige un mínimo de 80, el hardware moderno suele dar
  bastante más).

El propio uniform `sampler2D` no apunta a una textura directamente —
apunta a un número de unidad (`glUniform1i(location, 0)` = "lee de la
unidad 0"), y es `glActiveTexture` + `glBindTexture` quien decide qué
textura física está enlazada a esa unidad en cada momento. El estado de
ese enlace vive en registros/tablas internas del driver/GPU, no en la
VRAM donde sí vive el contenido real de los píxeles de la textura.

---

## Recursos recomendados para profundizar

No hay un único libro que cubra CPU + GPU + rendering al nivel de detalle
que venimos buscando — lista por área:

**CPU / RAM / arquitectura general:**

- *Computer Organization and Design* (Patterson & Hennessy, ed. RISC-V) —
  el clásico de referencia universitario, abordable.
- *What Every Programmer Should Know About Memory* (Ulrich Drepper, PDF
  gratuito) — jerarquía de memoria/caché desde la perspectiva de un
  programador de C/C++, directamente aplicable a optimizar el motor.
- *Computer Systems: A Programmer's Perspective* (Bryant & O'Hallaron,
  "CS:APP") — puente entre hardware y código en C.

**GPU específicamente:**

- *Programming Massively Parallel Processors* (Kirk & Hwu) — centrado en
  CUDA, pero la mejor fuente para entender el modelo de ejecución real de
  una GPU (warps, SIMT, jerarquía de memoria de GPU) en profundidad.
- Whitepapers de arquitectura de NVIDIA/AMD (gratuitos, buscar "NVIDIA
  Ampere/Ada architecture whitepaper", "AMD RDNA whitepaper") — densos,
  pero información de primera mano de los fabricantes.

**Rendering / gráficos — lo más aplicable a BrightEngine directamente:**

- *Real-Time Rendering* (Akenine-Möller, Haines, Hoffman) — el libro de
  referencia del campo; si solo se compra uno, es este.
- *Foundations of Game Engine Development* (Eric Lengyel, varios tomos:
  matemáticas, rendering...) — orientado específicamente a construir un
  motor desde cero.
- **learnopengl.com** (gratuito, online) — sigue prácticamente el mismo
  camino que hemos recorrido aquí (buffers → shaders → transformaciones →
  texturas), con más profundidad y llegando a temas que aún no hemos
  tocado (iluminación, sombras, PBR).

---

## Build system: quién hace qué (CMake, Ninja, MSBuild, el compilador)

Cuatro actores distintos, fácil confundirlos porque a veces se solapan:

- **CMake — configura y genera.** Lee los `CMakeLists.txt` del proyecto y
  **escribe** los ficheros de build de bajo nivel para otra herramienta
  distinta (`build.ninja`, `compile_commands.json`, `CMakeCache.txt`...).
  CMake no compila nada por sí mismo.
- **Ninja — ejecuta.** Lee el `build.ninja` que CMake ya generó y lanza de
  verdad los comandos de compilación/enlace que describe, en el orden y
  con las dependencias correctas. Ninja no genera nada, solo corre el
  plan ya escrito.
- **El compilador — hace el trabajo real.** Ninja invoca al compilador en
  cada paso — en esta máquina, **MSVC** (`cl.exe`, de
  `F:\Microsoft Visual Studio 2026`); en Linux sería GCC o Clang. Ni
  CMake ni Ninja compilan código — solo orquestan cuándo y cómo se llama
  al compilador.

**Cadena completa:** CMake configura y genera → Ninja ejecuta el plan,
llamando al compilador en cada paso → MSVC/GCC/Clang compila de verdad.

**¿Y MSBuild?** Es el motor de compilación propio de Microsoft — el que
ejecuta de verdad los `.sln`/`.vcxproj` que usa Visual Studio (y también
`dotnet build`). Es el motor detrás de la opción de CMake
`-G "Visual Studio 17 2022"`. No lo usamos aquí por dos motivos:

- **Motivo inmediato:** CMake 3.27 no tiene un nombre de generador para
  VS2026 (demasiado reciente) — sin ese generador no hay forma directa
  de que CMake escriba `.sln`/`.vcxproj` para MSBuild.
- **Motivo de fondo, más allá de ese problema puntual:** BrightEngine
  compila en Windows *y* Linux, y MSBuild no existe en Linux — así que
  usar Ninja en ambas plataformas da un único flujo de ejecución de
  build igual en los dos sitios (solo cambia el compilador, MSVC vs
  GCC/Clang, no la herramienta que orquesta). Además, el modo
  "Open Folder" de Visual Studio con CMake también suele usar Ninja por
  debajo, no el generador clásico de `.sln`.

**Generadores CMake, en general:** con `-G "Ninja"`, CMake escribe
`build.ninja`, ejecutas `ninja`. Con `-G "Unix Makefiles"`, escribe
`Makefile`s, ejecutas `make`. Con `-G "Visual Studio 17 2022"`, escribe
`.sln`/`.vcxproj`, los ejecuta MSBuild. Ninja es solo uno de varios
motores de ejecución posibles a los que CMake puede apuntar.

**De dónde viene Ninja:** lo creó Evan Martin en Google, hacia 2010,
porque `make` se estaba quedando lento compilando Chromium — el propio
parseo del Makefile y decidir qué estaba desactualizado ya tardaba
mucho en un proyecto tan grande. Ninja se diseñó deliberadamente
minimalista y muy rápido, sacrificando flexibilidad a propósito (sin
comodines, condicionales, ni lenguaje de script) — sus ficheros `.ninja`
están pensados para ser generados por otra herramienta (CMake, Meson,
GN...), no escritos a mano.

**Por qué generadores de una sola configuración (Ninja/Make) no tienen
carpetas `Debug`/`Release` automáticas:** el generador de Visual Studio
es multi-configuración — el mismo proyecto generado sabe construir varias
configuraciones sin reconfigurar, por eso necesita carpetas separadas.
Ninja es de una sola configuración: un directorio de build representa
**una única configuración a la vez**, la que se le dio a CMake vía
`CMAKE_BUILD_TYPE` al configurar. La convención habitual para tener
varias es usar **carpetas de build separadas por configuración** (p.ej.
`build-msvc-debug/`, `build-msvc-release/`), no reutilizar la misma
carpeta cambiando el tipo.
