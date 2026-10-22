# Renderer v2 cursor, camera and Performance menu update

## Update — October 22, 2026

The camera options are now combined into **Camera improvements (1.5x + slowdown fix)** in Mods. One checkbox enables the 50% increase in horizontal and vertical camera speed limits and prevents loss of horizontal boost during strong diagonal input at a vertical viewing limit with zoom off.

Base: `v2026.10.22-cde38af`; game: **CUSA00223 01.07**. All four Renderer v2 presets share the fix. Existing Camera speed 1.5x selections carry over; new users must enable the combined checkbox before launching.

Windows package validation and manual gameplay testing passed. The player confirmed unzoomed diagonal rotation stays fast. The slight slowdown while aiming remains unchanged. Vertical limits and initial acceleration remain active. Broader scene testing is still useful.

**Downloads are being prepared; the release remains unpublished while Linux builds and checks run.** The Linux build is available under this repository's Actions tab as **Camera update Linux**. No proprietary game files are uploaded or included.

### Earlier fixes included

- Invisible overlay cursor: menu, guide and save-dialog transitions use the overlay's own ImGui context.
- Performance tab crash: FPS labels now match 30, 60, 120, 240 and Unlimited, with a compile-time count check.
- Windows launcher: excludes incompatible MSYS2 `icuuc.dll` while retaining Linux ICU libraries.

### Windows update installation

After the download release is published, close the launcher and game, back up the package folder, and extract `renderer-v2-camera-complete-update.zip` into the folder containing `inFAMOUS Second Son.exe`. Replace matching files and retain the included `_cursor_camera_internal` folder. Open Mods and enable **Camera improvements (1.5x + slowdown fix)**. Changes apply on the next game launch.

Do not use the Windows ZIP on Linux. Linux packaging and validation are pending. The original build instructions remain available in the repository README.

### Validation

Windows: exact-release Renderer v2 build; overlay-context and five-value FPS dropdown checks; generated camera hook execution and signature rejection; both pitch boundaries and both pan directions; zoom, neutral, reversal, small-input, moving-pitch and invalid-value exclusions; frozen launcher selftest; combined-checkbox off/on/off checks across all four Renderer v2 presets. Linux CI adds a portable generated-bridge execution test using synthetic instruction fixtures, requiring no game dump. Successful Linux build and packaging results will be recorded when available.

## Português (Brasil)

**Atualização:** as duas opções agora estão reunidas em **Camera improvements (1.5x + slowdown fix)** na aba Mods. Uma única caixa ativa o aumento de 50% na velocidade da câmera e evita a perda de velocidade horizontal nas diagonais fortes ao atingir o limite vertical, sem zoom. As seleções antigas de Camera speed 1.5x são mantidas.

A redução de velocidade durante a mira com zoom permanece. Os limites verticais e a aceleração inicial continuam ativos. A compilação e os testes do Windows passaram, e o jogador confirmou a rotação diagonal rápida sem zoom. O pacote Linux está em compilação e validação; a publicação permanece em rascunho até essa etapa terminar.

Inclui as correções anteriores do cursor invisível, da aba Desempenho (30, 60, 120, 240 e Sem limite) e do empacotamento ICU do launcher Windows. Não inclui arquivos proprietários do jogo. Não use o ZIP Windows no Linux.
