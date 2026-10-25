# Adaptación SSE4A: EXTRQ en Intel

Cambios del fork, 25 de octubre de 2026. La tabla ISA de la instalación local identifica
los sitios calientes `0x89137d`, `0x890c8e`, `0x890aa7` y los restantes diez primeros
del registro como EXTRQ de registro, con salto directo o salto con instrucción reubicada.
Ambos caminos utilizan ahora una secuencia SSE2 directa. No es necesario regenerar la
tabla ISA ni borrar la caché de shaders.

## Implementación

Antes cada ejecución guardaba los registros que puede modificar una llamada SysV,
guardaba/restauraba flags, volcaba los operandos a la pila y llamaba a
`isa_emulate_sse4a`. Ahora:

- EXTRQ de registro captura ambos campos antes de modificar el destino, usa dos
  registros XMM temporales preservados y opera con desplazamientos y máscaras SSE2.
- EXTRQ inmediato usa desplazamientos constantes y no necesita temporales XMM.
- MOVQ de codificación legacy pone a cero el qword alto del XMM de destino y conserva
  la mitad alta del YMM, siguiendo el comportamiento AMD registrado por las pruebas.
- Los flags no cambian: hasta el contador de diagnóstico usa MOV/LEA/MOV en lugar de
  INC, evitando PUSHFQ/POPFQ. El contador sigue siendo aproximado bajo concurrencia,
  como el contador anterior sin LOCK; no es parte de los datos del juego.
- Se conservan los GPR, XMM ajenos, MXCSR y la red zone. Los operandos constantes
  están alineados según su dirección efectiva en el stub. La cola compartida sigue
  ejecutando la instrucción reubicada y saltando a la dirección original de retorno.
- INSERTQ y los sitios INT3 conservan el fallback anterior. `TLG_SSE4A_INLINE=0`
  selecciona también el fallback anterior para EXTRQ, útil para comparaciones A/B.

El registro identifica la versión activa con `ISA: EXTRQ adaptation inline SSE2`.

## Pruebas y mediciones

En el i7-12700H local, pruebas del código generado (JMP y reubicación) contra el
modelo y los resultados AMD incluidos en el proyecto: todos los registros XMM,
alias de operandos, campos de longitud e índice, longitud cero, desbordamiento del
campo, flags, GPR, YMM superiores, red zone y arena de stubs desalineada. Incluye
las 4096 combinaciones de campos inmediatos y de registro, con y sin alias.
No hay una ejecución de comparación contra EXTRQ nativo en esta CPU Intel.

Microbenchmark local de un millón de ejecuciones por vuelta, mejor de tres,
incluyendo la misma instrumentación de conteo en ambas versiones:

| Variante | Fallback anterior | Inline SSE2 |
|---|---:|---:|
| EXTRQ de registro | 19,39 ns/op | 2,51 ns/op |
| EXTRQ inmediato | 17,65 ns/op | 1,25 ns/op |

Estos valores son un microbenchmark y **no son una multiplicación de los FPS**.
Las expulsiones de texturas, la presentación y los costes del renderizador siguen
requiriendo mediciones separadas en la misma escena.

```text
isa-sse4a-test --jumps-only --benchmark
TLG_SSE4A_INLINE=0 isa-sse4a-test --jumps-only
isa-emulate-test
isa-stub-test ../tests/data/isa_fixture.bin
isa-f16c-test
```

También pasaron las 24 pruebas Python de red zone y las 25 de generación de tabla ISA.

### Limitación preexistente de la prueba completa en Windows

Sin `--jumps-only`, el fixture INT3 no protegido falla su comparación de red zone
en este Windows tanto con la versión anterior (`TLG_SSE4A_INLINE=0`) como con la
nueva: `extrq xmm0,xmm0 ... mode 2`, diferencia solo en red zone. Esta optimización
no cambia INT3 ni elimina esa aserción. El modo explícito `--jumps-only` permite
probar todos los caminos optimizados sin presentar ese fallo anterior como resuelto.
Las pruebas generales de stubs y F16C pasaron por separado.

### Comprobación de arranque local

Durante el smoke test se actualizó externamente NVIDIA de 596.36 a 621.01. Vulkan
solo exponía temporalmente la Intel integrada: se detuvo esa ejecución y se restauró
la caché NVIDIA desde el respaldo independiente. Se comprobó la disponibilidad del
manifiesto NVIDIA del paquete instalado sin modificar el registro de Windows ni
dejar overrides de driver en el launcher. La instalación del driver seguía abierta.
Por ello no se presenta esa ejecución como validación de FPS del juego con la nueva
adaptación. Hay que terminar la instalación y volver a probar tras reiniciar.

## Procedencia

Se conserva el emulador y la infraestructura de reescritura del proyecto original.
La semántica de referencia y los resultados AMD provienen de
`port/src/isa_emulate.c` y `port/tests/test_isa_sse4a.c`, que citan el AMD64
Architecture Programmer's Manual, volumen 4. El fork añade los emisores SSE2,
el barrido de campos de los stubs y el microbenchmark comparativo.

Referencia oficial: AMD, [AMD64 Architecture Programmer's Manual, volumen 4,
documento 26568](https://docs.amd.com/v/u/en-US/26568_3.26_APM_Vol4).
