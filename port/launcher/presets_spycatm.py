# SPDX-License-Identifier: GPL-2.0-or-later
"""Personal quality/performance profile for the i7-12700H and RTX 3050 Ti Laptop 4 GiB.

Uses this fork's bounded uploads and shared compiler cache. It is a hardware-targeted
starting point, not evidence that this machine achieves stable 30/40 FPS.
"""
from presets_current import CURRENT

SPYCATM = {
    'id': 'spycatm',
    'exported': '2026-10-25',
    'names': {'en': 'SpyCatM', 'pt': 'SpyCatM'},
    'credit': 'DavidPilaguano · SpyCatM',
    'credit_url': 'https://github.com/DavidPilaguano/iss_pc',
    'what': {
        'en': 'RTX 3050 Ti Laptop 4 GB / i7-12700H: precise effects, modest LOD reduction, '
              'shared shader cache and bounded transfers. No guaranteed FPS target.',
        'pt': 'RTX 3050 Ti Laptop 4 GB / i7-12700H: efeitos precisos, redução moderada de LOD, '
              'cache compartilhado e transferências limitadas. Sem garantia de FPS.',
    },
    'settings': dict(CURRENT['settings'], vsync=False, show_fps=True,
                     precompile_shaders=True, vram_auto=False, vram_budget_mb=3000),
    'items': dict(CURRENT['items'], **{
        'menu:fps_limit': '0',
        'menu:frames_queued': '2',
        'menu:lod_scale': '0.85',
        'menu:region_lod': '1',
        'menu:region_fade': '1',
        'menu:near_clip': '10',
        'menu:far_clip': '10000000',
        'menu:mip_lod_bias': '0',
        'TLG_ASYNC_SHADERS': '0',
        'TLG_DEFERRED_READBACK': '1',
        'TLG_STALE_READBACK': '0',
        'TLG_BACKGROUND_WARMUP': '1',
        'TLG_SHARED_PIPELINE_CACHE': '1',
        'TLG_SHADER_ON_DEMAND': '1',
        'TLG_STAGING_KEEP_MB': '128',
        'TLG_PREP_WORKERS': '8',
        'TLG_COPY_THREADS': '4',
    }),
}
