**English** · [Português (Brasil)](pt-BR/decisions.md)

# Decisions

Dated record of the decisions that shaped the engine, newest first.

## 2026-10-16: Direct3D 12 stays with D3DMetal

A one-day spike looked for a way to play Direct3D 12 without the user's own D3DMetal (Game Porting Toolkit), and
found none. Playable Direct3D 12 still needs D3DMetal, and WinHub's next step is the app-side guide to the Game
Porting Toolkit. Measured on revision 5, on Apple silicon with MoltenVK 1.4.0:

- **Wine's vkd3d** (the build's own): with Wine's `dxgi` it creates a device, but only at feature level 11_0 (MoltenVK
  lacks geometry shaders, stream output, depth clip and cull distance). With "Automático" (DXMT's `dxgi` first) it
  cannot create a device at all: DXMT v0.80's `dxgi` answers `E_NOINTERFACE` to the adapter interface vkd3d asks for.
- **vkd3d-proton v3.0.1 on MoltenVK:** stops at the requirements. MoltenVK 1.4.0 has no `VK_EXT_transform_feedback`,
  no `robustBufferAccess2` or `nullDescriptor` (`VK_EXT_robustness2`), and does not support single-texel alignment
  for texel buffers (it reports 16 bytes). Only MoltenVK's features were probed; vkd3d-proton was not built.
- **DXMT's own D3D12** (official commit `50102f9`, LGPL-2.1, built with `-Denable_d3d12`): the smoke program works in
  64-bit (device, clear and read back, 30 presents), and it also fixes the "Automático" blocking. But it is early:
  feature level capped at 11_1, shader model 5_1, DXIL shaders (SM 6) refused with `E_NOTIMPL`, and so are geometry,
  hull, domain and stream-output shaders. 32-bit aborts. The Direct3D 11 smoke is identical to revision 5's.

Why each path stops: Wine's vkd3d tops out at 11_0 on this Vulkan layer; vkd3d-proton needs Vulkan features
MoltenVK does not have and the project does not patch MoltenVK; DXMT's D3D12 cannot run what current games ship (DXIL
and tessellation are common).

Skipped on purpose: playing a real Direct3D 12 game. The sources above already show that modern games need DXIL and
often tessellation, so the run would have failed on the first shader and measured nothing new.

Follow-up, recorded as a gap and not as a decision to ship (see [Gaps](gaps.md)): revision 5's "Automático" blocks
even Wine's vkd3d, so a Direct3D 12 program cannot create a device unless D3DMetal is set. Two options to evaluate for a
revision 6: move to the newer DXMT (LGPL; fixes it and adds basic D3D12 at 11_1 with DXBC shaders, but replaces the
whole Direct3D 11 path), or a narrow fix (a small patch so DXMT's `dxgi` gives vkd3d its adapter, or `winhubcompat`
not putting DXMT's `dxgi` first for Direct3D 12 programs).

The spike's tools stay on the `spike-d3d12` branch: `tests/d3d12-smoke.c`, `tests/vulkan-features.c` and
`scripts/build-dxmt-d3d12-spike.sh`. No binary was published and the installed engine was not touched.

## 2026-10-15: Game videos through GStreamer with VideoToolbox

Revision 5 plays game videos: Wine's Media Foundation hands them to `winegstreamer`, which needs GStreamer, and the
official build has none (CrossOver bundles it). The engine builds the GLib 2.78.0 and GStreamer 1.24.4 in
CrossOver's LGPL sources, and H.264 and AAC are decoded by macOS itself (VideoToolbox and AudioToolbox, through
gst-plugins-bad's `applemedia`) instead of by FFmpeg. Why:

- the codecs are the system's, so the engine redistributes no patent-encumbered decoder and no GPL code
  (`-Dgpl=disabled`; no FFmpeg, gst-libav, x264 or gst-plugins-ugly);
- hardware decoding comes for free, and the overlay grows by only 3.7 MB compressed;
- the only new downloads are pcre2 10.42 (GLib needs it) and gvdb at a pinned commit (CrossOver's archive ships it
  empty); both are pinned and ship with the release sources.

Left out: WMV/WMA, MPEG-1 and Theora, which macOS cannot decode (see [Gaps](gaps.md)). Bink videos are not
affected: games play them with their own code, not through Media Foundation. The GStreamer libraries live in `lib/`
beside the base build's, so `overlay.replace` only gained `lib/gstreamer-1.0`.

`winhubcompat` sets `GST_PLUGIN_SYSTEM_PATH_1_0` and `GST_REGISTRY_1_0` instead of WinHub doing it, so a WinHub
that predates revision 5 plays videos unchanged, and a bottle's own values still win.

## 2026-10-12: DXMT's NVIDIA extensions ship, off unless the bottle asks

Revision 3 builds DXMT's `nvapi64` and `nvngx` (meson `enable_nvapi`, `enable_nvngx`; NVIDIA's MIT NVAPI headers
at the commit DXMT pins). They live in `lib/dxmt` and are only reached with DXMT active. DXMT itself keeps them
inert unless `DXMT_ENABLE_NVEXT=1`, which WinHub sets per bottle, so a game sees "no NVIDIA device" by default, as
before. No `nvngx` redirect is needed for DXMT: Wine resolves `system32\nvngx.dll`, which is not on disk, to the
builtin in the DLL path.

## 2026-10-12: Documentation describes the reverse engineering openly

The project documents what was studied in CrossOver and how it is reproduced, in the style of other clean-room
re-implementations. Decompiled code, addresses, internal symbol names and analysis logs stay out of the
repository. See [Reverse engineering](reverse-engineering.md).

## 2026-10-12: Keep upstream Wine's user name, profile and Desktop

CodeWeavers' tree reports every user as `crossover`. A WinHub bottle made with the official Wine lost Steam's
saved login when it ran the engine, because Wine created a new `C:\users\crossover` profile. `patches/wine/0003`
restores upstream behaviour.

## 2026-10-12: The rule database is signed with Ed25519

WinHub downloads `data/compatdb.json` from `main` and installs it only when its signature checks out against the
public key built into the app, and when it is newer than the installed one. The private key never enters the
repository. See [SECURITY](https://github.com/AngeloResplandes/winhub-engine/blob/main/SECURITY.md).

## 2026-10-12: DXVK only in 64-bit processes

32-bit processes back Vulkan buffers with host memory, which MoltenVK refuses (CodeWeavers' MoltenVK too).
`winhubcompat` treats DXVK as unusable there, so they fall back to DXMT.

## 2026-10-11: Build Wine from CrossOver's LGPL sources (revision 2)

Revision 1 was the official WineHQ build with a patched `winemac.so`. Revision 2 builds all of Wine from
`crossover-sources-26.3.0`, which already has msync, WoW64 and the `winemac.drv` hooks DXMT and D3DMetal need,
and keeps the official build only as the base for libraries, wine-mono and wine-gecko. Using code CodeWeavers
publishes under the LGPL is allowed; their closed parts are not used.

## 2026-10-11: Pick the backend inside the engine

Instead of copying DXMT or DXVK into each bottle (revision 1, as CrossOver did until version 24), `winhubcompat`
puts the chosen backend's folder first in Wine's DLL search path, per process. Bottles stay clean and a game can
have its own backend through a rule.

## 2026-10-11: D3DMetal only from the user

Apple's license does not allow redistributing D3DMetal. The engine supports it when the user points WinHub at
their own Game Porting Toolkit; it is never shipped or downloaded by the project.

## 2026-10-11: DXMT v0.80

The last MIT-licensed release of DXMT; later versions are LGPL.
