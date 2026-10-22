# Renderer v2 cursor, camera and Performance menu update

Base release: `v2026.10.22-cde38af`.

## English

- Fixes invisible overlay cursors by updating the overlay's own ImGui context for menu, guide and save-dialog transitions. All Renderer v2 presets share this code.
- Fixes the Performance tab FPS dropdown: five labels match 30, 60, 120, 240 and Unlimited. A compile-time size check prevents another label/value mismatch.
- Adds an optional Camera speed 1.5x entry to Mods for CUSA00223 01.07. It defaults off, persists in mods.json and applies on the next game launch. The guarded runtime hook increases actual horizontal and vertical camera speed parameters without multiplying controller axes or editing game files.
- Adaptive slowdown, acceleration and pitch limits remain unchanged. If the mod appears inactive on first launch, close and reopen the game; this was a reported workaround in the previous development build, not an intended two-launch requirement.
- Excludes incompatible MSYS2 icuuc.dll from the Windows Qt bundle while retaining Linux ICU libraries. ISS_SPEC_CONTENTS optionally selects a private library folder; the default remains _internal.

Build with the repository's normal instructions. On MSYS2 CLANG64, run `TLG_RENDERER_V2=1 bash build.sh` from port/, then use normal packaging. Open Mods -> Camera speed 1.5x before launch. Open the overlay with Insert or Xbox LT + right-stick click. This branch contains source, not prebuilt ZIPs or proprietary game files.

Windows validation: exact-release Renderer v2 build passed; actual menu function tested with both ImGui contexts; actual FPS dropdown drawn headlessly at all five values; generated camera code executed for identity, 1.5x and independent axes, with invalid-scale/signature/truncation rejection; frozen Qt launcher selftest passed after reproducing and fixing the ICU failure. Installed frozen-launcher checkbox off/on/off tests passed for Renderer v2, Native Passes, 8 GB and 12 GB presets while preserving their native-pass/VRAM settings.

In-game visual confirmation, broader gameplay tests and Linux validation of this combined branch remain pending. The display FPS limit is distinct from engine frame rate.

## Português (Brasil)

Corrige o cursor invisível no Renderer v2 usando o contexto ImGui correto e corrige a aba Desempenho: os cinco rótulos correspondem a 30, 60, 120, 240 e Sem limite, com verificação da quantidade durante a compilação.

Adiciona Camera speed 1.5x à lista de Mods para CUSA00223 01.07, desativado por padrão e salvo no mods.json. Ative antes de iniciar o jogo. A alteração aumenta os parâmetros reais de velocidade horizontal e vertical, sem multiplicar os eixos do controle nem editar o ELF. A versão e as assinaturas originais são verificadas. A redução adaptativa, a aceleração e os limites verticais permanecem. Se parecer inativo na primeira execução, feche e abra o jogo novamente; esse foi um contorno relatado, não um requisito intencional.

Os quatro perfis do Renderer v2 compartilham o runtime: padrão, Native Passes, 8 GB e 12 GB. Abra o menu com Insert ou LT + clique no analógico direito no Xbox. O pacote Qt do Windows exclui a icuuc.dll incompatível do MSYS2; as bibliotecas ICU do Linux continuam elegíveis.

A compilação e os testes locais de dropdown, contexto do cursor, código da câmera e launcher congelado passaram no Windows. A confirmação visual no jogo, testes mais amplos de gameplay e a validação Linux desta combinação ainda estão pendentes. Este branch contém código-fonte, sem arquivos proprietários do jogo.
