# Atribución y procedencia de este fork

Este es un fork personal mantenido por [DavidPilaguano](https://github.com/DavidPilaguano).
El port original de inFAMOUS Second Son es obra de **HayatoG y los colaboradores de
[HayatoG/iss_pc](https://github.com/HayatoG/iss_pc)**. Este fork conserva su historial,
créditos, avisos y licencias; no atribuye al mantenedor del fork la autoría del port.

## Punto de partida y cambios propios

Base de estos cambios: commit
[`88e912fa8323`](https://github.com/HayatoG/iss_pc/commit/88e912fa8323),
rama local `developer`. Cambios del fork preparados el 25 de octubre de 2026:

- Metadatos de páginas dispersos, menor granularidad de residencia en GPU de 4 GiB
  y transferencias divididas y empaquetadas con memoria temporal acotada.
- Retención compartida de staging, presupuesto de VRAM actualizado y carga opcional
  de shaders a demanda.
- Caché compartida entre compiladores y guardado sin duplicar esa caché en Vulkan.
- Detección de los idiomas presentes en CUSA00004 y sección «Mi equipo» del launcher.
- Pruebas específicas, muestreador de memoria y documentación de resultados y límites.
- Lecturas independientes de paquetes en Windows, subidas de zonas con dos bloques
  alternos y menor frecuencia de consultas rutinarias del recolector. Véase
  [ZONE_STREAMING.md](port/docs/ZONE_STREAMING.md) para la procedencia técnica,
  las pruebas y los límites de estos cambios.
- Conexión de ajustes en vivo del launcher al runtime, escrituras atómicas del INI
  y accesos locales que respetan la configuración editable. Ver
  [LAUNCHER_SETTINGS.md](port/docs/LAUNCHER_SETTINGS.md).

El objetivo de 8 GB y 30 FPS estables **no está demostrado**. Los resultados medidos,
las condiciones y las limitaciones están en
[RAM_OPTIMIZATION.md](port/docs/RAM_OPTIMIZATION.md).

## Código y componentes anteriores

La cadena de procedencia del núcleo y sus revisiones está documentada por el proyecto
original en [port/UPSTREAM.md](port/UPSTREAM.md): gow3_pc, bloodborne_pc y
[shadPS4](https://github.com/shadps4-emu/shadPS4), entre otros.
Se conserva ese documento y la [licencia del núcleo de vídeo](port/gpu/shadps4/LICENSE).
Los submódulos y sus repositorios originales se enumeran en [.gitmodules](.gitmodules).
Cada dependencia, biblioteca, fuente e imagen conserva los avisos y la licencia que
acompañan a sus archivos. Los parches locales de FSR-Vulkan se mantienen en
[su directorio de parches](port/gpu/patches/fsr-vulkan/README.md).

El código del port mantiene **GPL-2.0-or-later**, según
[port/README.md](port/README.md) y [port/LICENSE](port/LICENSE).
Este fork no cambia las licencias de las dependencias.

## Bibliografía técnica

Estas referencias informaron el diseño; no son una afirmación de autoría ni de respaldo
del fork por sus autores. La aplicación concreta se explica en
[la bibliografía de optimización](port/docs/RAM_OPTIMIZATION.md#bibliography-and-application).

- Erfan Ahmadi, *Keeping your staging buffer fixed size!*, Vulkanised 2023:
  [presentación de Vulkan](https://www.vulkan.org/user/pages/09.events/vulkanised-2023/vulkanised_2023_keeping_your_staging_buffer_fixed_size.pdf).
- Khronos Group, *Vulkan Guide: Memory Allocation*:
  [documentación](https://docs.vulkan.org/guide/latest/memory_allocation.html).
- Vulkan Memory Allocator / GPUOpen, *Staying within budget* y *Recommended usage patterns*:
  [presupuestos](https://gpuopen-librariesandsdks.github.io/VulkanMemoryAllocator/html/staying_within_budget.html),
  [patrones de uso](https://gpuopen-librariesandsdks.github.io/VulkanMemoryAllocator/html/usage_patterns.html).
- Khronos Group, referencia de Vulkan para sincronización y serialización de cachés:
  [vkCreatePipelineCache](https://docs.vulkan.org/refpages/latest/refpages/source/vkCreatePipelineCache.html),
  [vkGetPipelineCacheData](https://docs.vulkan.org/refpages/latest/refpages/source/vkGetPipelineCacheData.html),
  [vkMergePipelineCaches](https://docs.vulkan.org/refpages/latest/refpages/source/vkMergePipelineCaches.html).

## Preset compartido como referencia

El usuario aportó un preset titulado «RTX 3060 - Ryzen 5 5600x», con fecha
`2026-10-20`, para CUSA00223 y la versión `v2026.10.20-604039c`. La captura de la
conversación lo atribuye al usuario **Kazu**. Se consultó como referencia de ajustes;
no se importó íntegramente ni se presenta como un preset creado por este fork.
No se dispone de un enlace público verificable al mensaje ni de una licencia del preset,
por lo que no se redistribuyen aquí el archivo completo ni la captura de la conversación.

## Datos que no se publican

El juego original y sus recursos pertenecen a sus respectivos titulares. Este fork
publica cambios de código, pruebas sintéticas y documentación, no dumps del juego,
guardados, shaders compilados, cachés, ejecutables locales ni credenciales.
Estas exclusiones están en [.gitignore](.gitignore).
