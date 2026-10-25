# Controles del launcher y aplicación en el juego

Verificación del fork, 25 de octubre de 2026. Los controles no se limitan a cambiar
su etiqueta: la pantalla Gráficos llama a `Graphics.setValue`, guarda el valor en
`settings.json` o `tlg.ini` y el runtime lo consume. Las pruebas de Qt comprueban
esa escritura; `graphics-settings-test` comprueba lectura, aplicación, restauración
de valores predeterminados y respeto de overrides explícitos.

## Correcciones

- El runtime vuelve a leer los ajustes en vivo cada 250 ms, incluso con el overlay
  oculto. Solo aplica claves conocidas que hayan cambiado; no vuelve a inicializar
  el motor ni las opciones de arranque. Si se elimina una clave, recupera el valor
  predeterminado. Los overrides de entorno explícitos conservan su precedencia y
  quedan indicados en el registro.
- Los controles de pantalla, VSync y contador de FPS se escriben también en el INI;
  antes quedaban solo en JSON hasta el siguiente lanzamiento.
- La escritura del INI del launcher es atómica: el lector no ve un archivo truncado.
  En Windows se reintenta brevemente si un lector mantiene abierto el archivo.
- 24 FPS, que ya figuraba en el launcher, ahora es aceptado por el runtime y por el
  menú del juego. Antes se interpretaba como un límite desconocido, es decir, sin límite.
- Los accesos locales `Launcher-inFAMOUS.exe` y `Jugar-inFAMOUS.exe` se generan desde
  `port/launcher/windows_entry.c`: ya no fuerzan idioma, precisión, presupuesto de
  VRAM ni opciones avanzadas por encima de lo guardado. La instalación local conserva
  sus valores efectivos de staging/caché en el INI editable. `port/out/entry.ini`
  solo identifica la carpeta de datos del usuario (`[launcher]`, `user_dir`); no
  contiene un preset. Los accesos buscan el entorno local `out/test-python` y la
  instalación MSYS2 CLANG64 usada para construir el runtime. Sus binarios y la
  configuración particular se mantienen fuera del repositorio mediante `.gitignore`.

## Qué modifica cada control de la captura

| Control | Consumidor real | Alcance |
|---|---|---|
| LOD distance | `tlg_view.cpp`, tabla LOD del ejecutable verificado | Multiplica los factores de distancia; el motor conserva su ajuste dinámico de calidad. |
| Regions' LOD_ADJUST | Stubs de `view_hook.h` | Peso del ajuste LOD de las regiones. |
| Regions' FADE_DISTANCE | Stubs de `view_hook.h` | Distancia de mezcla de parámetros de región; no es una reducción directa de niebla. |
| Near/Far clip | `tlg_view.cpp`, valores globales de cámara | Predeterminados para cámaras nuevas; cámaras existentes o regiones pueden conservar valores propios. No es un límite global de streaming. |
| Texture LOD bias | `Rasterizer::BindResources` | Bias de muestreo de texturas en dibujos gráficos, redondeado a octavos; sin efecto apreciable en texturas sin mipmaps. |

El hook de distancia se instala únicamente cuando coincide la firma del ejecutable.
En la copia local CUSA00004 se verificó previamente el mensaje `Draw distance: ready`.
Esto no prueba un cambio visible en todas las cámaras ni constituye una medición de FPS.

## Comprobar la aplicación

Al cambiar una opción en vivo desde el launcher, el registro de la partida contiene,
por ejemplo, `Settings live: lod_scale=0.6`. Si existe un override externo aparece
`remains overridden by ...`. Las opciones de GPU, compilación, renderer y memoria que
se fijan al arrancar siguen necesitando reiniciar el juego. Los rótulos «IN GAME»
del launcher original indican también que la opción existe en el menú Insert;
no constituyen una promesa de que cualquier subsistema se pueda reconstruir en vivo.

Estas pruebas verifican el recorrido de configuración y sus consumidores en código.
La comparación visual de cada opción en cada escena sigue siendo una validación aparte.
