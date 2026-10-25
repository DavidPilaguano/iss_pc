# Carga de zonas y caídas de FPS

Cambios del fork del 25 de octubre de 2026. Se conserva la precisión de GPU y la
calidad configurada; no se modifica la resolución, el detalle ni la velocidad del motor.

## Evidencia anterior al cambio

En la prueba local de mundo abierto hubo ventanas de unos 17 FPS y cuadros frecuentes
de unos 66 ms sin compilación de shaders. También se registraron expulsiones repetidas
de texturas bajo presión de memoria y una subida de 553 MiB dividida en bloques de
64 MiB. Una captura corta de carga contiene compilaciones y esperas de lectura de
GPU; esa captura no constituye una comparación de mundo abierto ni demuestra por
sí sola la causa de cada caída.

## Cambios

- **Transferencias grandes con dos bloques alternos.** En GPU pequeñas, las subidas
  de más de 64 MiB utilizan hasta dos bloques de 64 MiB. Antes se terminaba y esperaba
  toda la entrega anterior antes de copiar cada bloque siguiente. Ahora se entrega
  el bloque a la GPU y la CPU puede llenar el otro. Se espera el tick del bloque
  concreto antes de reutilizarlo. Los bloques permanecen retenidos hasta finalizar
  el trabajo de preparación; su liberación respeta el timeline. No se amplía la
  retención de staging configurada. La memoria en uso durante una subida puede ser
  mayor que con un solo bloque: hasta 128 MiB para estas dos ventanas, además de
  otras transferencias pendientes. El último bloque sigue la entrega normal del
  renderizador y los datos no se descartan.
- **Lecturas independientes de paquetes en Windows.** Los archivos de solo lectura
  de `/app0` y `/hostapp` obtienen un handle adicional `FILE_FLAG_OVERLAPPED`. Las
  lecturas posicionadas concurrentes no mueven ni restauran un cursor compartido.
  Cada operación conserva su evento y espera su finalización antes de usar los datos.
  Se mantiene el buffer intermedio que permite copiar a páginas protegidas por la
  GPU. Los guardados y las escrituras conservan su ruta anterior. Si no se puede
  abrir el handle adicional se utiliza la ruta anterior; `TLG_STREAM_READ=0` la
  selecciona explícitamente para comparaciones. No se añade una caché de archivos.
- **Menos trabajo repetido del recolector.** Las consultas rutinarias de presupuesto
  y las pasadas por imágenes se separan por al menos 16 ms en lugar de ejecutarse en
  cada envío de comandos. Se mantienen umbrales, edades y límites de expulsión. Las
  escrituras de backing se aplican en cada envío; la recuperación ante fallos de
  asignación sigue siendo inmediata y el recolector de samplers conserva su ritmo.

## Verificación

- `stream-read-test`: archivo sintético, seis lectores simultáneos sobre el mismo
  paquete, bytes exactos, posición de lectura, EOF, errores de offset, ausencia de
  fuga de handles, APIs reales de archivos del guest, ruta de fallback y guardados.
- `upload-streaming-test`: prueba real de Vulkan con dos bloques, 25 entregas y una
  cola parcial; espera la finalización antes de reutilizar cada bloque y compara
  todos los bytes recibidos. Pasó en la RTX 3050 Ti Laptop. Es una comprobación del
  protocolo de transferencia; no reproduce toda la escena ni mide FPS del juego.

Ambos targets están en `port/gpu/CMakeLists.txt` (el de archivos es solo para Windows).
La compilación completa del runtime es necesaria para probar estos cambios.

Los registros nuevos identifican las rutas mediante:

```text
Runtime: independent overlapped package reads enabled (TLG_STREAM_READ)
Uploading ... MB in parts of 64 MB (2 staging slots)
```

## Límites de lo demostrado

No se ha demostrado todavía una mejora cuantificada de FPS ni 40 FPS estables.
La rama de inFAMOUS no incluye un parche integrado validado para desbloquear el
framerate del juego (`port/scripts/infamous_patches.py`). Un límite de presentación
mayor no crea cuadros ni sustituye ese trabajo. Evitar esperas de carga puede reducir
tirones, pero no garantiza corregir todos los costes del renderizador en mundo abierto.
Hace falta comparar el mismo guardado y recorrido, con caché de shaders caliente,
durante varios minutos, midiendo tiempos por cuadro además de FPS medios.

## Fuentes

- Microsoft, [ReOpenFile](https://learn.microsoft.com/en-us/windows/win32/api/winbase/nf-winbase-reopenfile):
  reapertura de un archivo con flags de I/O independientes.
- Microsoft, [GetOverlappedResult](https://learn.microsoft.com/en-us/windows/win32/api/ioapiset/nf-ioapiset-getoverlappedresult):
  esperar la operación concreta, manteniendo vivos el buffer, el evento y `OVERLAPPED`.
- Khronos, [Vulkan synchronization examples](https://docs.vulkan.org/guide/latest/synchronization_examples.html):
  sincronización entre accesos del host y transferencias del dispositivo.
- [Bibliografía previa de memoria y staging](RAM_OPTIMIZATION.md#bibliography-and-application).
