# Changelog

**English** · [Português (Brasil)](CHANGELOG.pt-BR.md)

What changed from one release to the next. Release notes on GitHub are the same text with download and install
instructions; runtime patches are listed one by one in [`engine/README.md`](engine/README.md).

## Versioning

Versions are `0.MINOR.PATCH` ([Semantic Versioning](https://semver.org/) for a project that is not finished yet):

- **MINOR** (0.**4**.0) goes up for a release with new features or performance work. It may ask you to rebuild the game.
- **PATCH** (0.3.**1**) goes up for a release with bug fixes only.
- **1.0.0** is reserved for the complete port: the whole game playable at a locked 60 fps, with no known blocking bugs.

The tag of a release is `v` + its version, and the launcher inside it reports the same version: the launcher's
update check compares the two.

**Renumbering (2026-10-31).** The first releases were tagged v1.0.0, v2.0.0, v3.0.0 and v3.0.1 while their titles
said v0.0.x. They are now v0.1.0, v0.2.0, v0.3.0 and v0.3.1, with the same files and contents. A launcher from those
releases (it reports 1.0.0 to 3.0.1) does not see 0.x releases as newer: **install v0.4.0 once from its setup over the
existing installation**; automatic updates work again from there on. Your game build, discs and saves are kept.

## [v0.7.1] — 2026-11-16

Every path an installation records about itself is now relative, including the card, the saves folder, the texture pack
and the regional builds the launcher keeps. v0.7.0 made the build folder and the disc portable; the regional build
records, the recompiler's sync state and the settings of the boot profile were still stored absolute, so a copied
installation could keep pointing at the folder it came from. Rebuild the game when the launcher asks (patch 25 changed).

### Portable records
- **Regional builds** (`data/<game>/regional-builds.json`) record `owner`, `elf`, `runner`, `source` and `dataDir`
  relative to the project, and the launcher resolves them there: a registered Europe or Japan build keeps working after
  the installation moves.
- **The recompiler's sync state** records its destination the same way, so a moved installation syncs into its own
  folder instead of the old one.
- **The boot profile** the launcher writes (`gow-native.boot`) records the disc, the memory card, the saves folder, the
  save assets, the VU1 capture folder, the texture pack, the texture-name list and the mod VFS table/log relative to the
  runner's folder while they live inside the installation. Patch 25 resolves them against that folder and uses an
  absolute value exactly as recorded, so the game finds its card and saves after the installation is copied or moved.
  The pipeline writes its first-run profile the same way.
- A path outside the project, or on another drive, stays absolute: no portable record can describe it, and old records
  that still hold absolute paths keep working.

### Development
- `verify` builds and runs `gow_boot_profile_test`, which covers the boot profile reader now living in the runtime's
  `runtime/gow_boot_profile.h` (header-only, added by patch 25): a relative path setting, an absolute one, a setting
  that is not a path and a legacy profile without path settings.
- The release workflow gives its publish job one `GITHUB_TOKEN`: the auto-release action reads it and `gh` falls back to
  it, so publishing no longer needs a second variable with the same value.

## [v0.7.0] — 2026-11-16

The installation now moves with its folder: the launcher, the setup and the pipeline record their paths relative to
the project, so copying or moving an installation no longer loses its build folder, its disc or its languages.
Rebuilding the game is required (patch 25 changed how the runner finds the disc it is given).

### Portable installation
- The build folder (`data/build-root.txt`) is recorded relative to the project, and the launcher, the uninstaller and
  the pipeline resolve it there: the same installation copied to another folder, or renaming that folder, keeps its
  build. A path outside the project (the short `C:\gow-build` fallback for deep paths, or another drive) stays absolute.
- The disc the launcher records in `gow-native.boot` and the ELF paths in `data/<game>/languages.json` are recorded the
  same way: the disc relative to the runner's folder, resolved by `gow-session` (patch 25), which is why the game has to
  be rebuilt. A moved installation boots its own disc instead of reporting it missing.
- CMake's cache is still not portable, so the pipeline checks the cache's source folder: a copied or moved installation
  builds again in `build-<hash>` and keeps the previous build folder and the player data untouched.

### Launcher
- **Moved installation:** a build folder inside the installation whose CMake cache names another project folder is
  reported as moved. The Install page explains it and offers the rebuild, and the Home status line says so instead of
  "not installed", which is what a copied installation used to look like.
- **Build folder > Change:** an existing installation can point its build at the project folder without rerunning the
  setup. Inside the installation the record is relative again; outside it, absolute. The picker refuses the
  installation folder itself, a base whose build path would exceed the pipeline's budget and a cache made by another
  copy, and maps a picked `build-clang` back to `build`. It never moves an existing build, because CMake's cache cannot
  leave its folder. Windows only: on Linux the pipeline always builds under `data/build-linux/<game>`.
- The new texts exist in the nine launcher languages (English, Portuguese, Spanish, French, German, Italian, Russian,
  Simplified Chinese and Turkish).

## [v0.6.1] — 2026-11-13

Stable release of the 0.6 series: everything in v0.6.0-rc.1 plus the changes below. Rebuilding the game is required
(new runtime patches); the launcher does it from the Install page. v0.6.0-rc.1 launchers report 0.6.0, so they are offered 0.6.1 as an update.

### Keyboard and mouse
- Rotation QTEs (#34), validated in the ship cabin minigame: the key guide is short and stable. Half turns show two
  keys, A > D or D > A, and the stick sweeps the arc under the bottom or over the top the way the prompt turns; full
  turns show W with a circular arrow: hold it and the stick keeps turning that way. The guide no longer follows the
  arrow's animation or flips direction between frames, which had made full turns and some half turns fail
  (`zzz-329`, `zzz-332`). The Accessibility "Circle" gesture starts at the guide's key and turns the prompt's way.
- Accessibility > automatic QTE now also performs recognized stick turns (half and full circles) with keyboard or
  controller, besides pressing buttons (`zzz-333`). Still off by default; shake prompts stay manual.
  See [design and tests](docs/research/keyboard-qte-rotation-2026-11-13.md).

### Launcher
- Pre-release launchers can report their suffix (`GOW_PRERELEASE=rc.1` when building gives `0.6.1-rc.1`), and the
  update check orders versions like semantic versioning, so testers of a release candidate are offered the final
  release. Launchers of v0.6.0-rc.1 report plain `0.6.0` and therefore only see a newer version number.
- QTE and accessibility options are translated into German, French, Italian and Russian; the "Circle" stick gesture
  text explains that it follows the key guide's arrow.

### Performance
- The setup ships a profile-guided optimisation (PGO) profile trained on this exact patch set (198,964 functions):
  new installs and rebuilds use it without a training step.
- A GS frame-latency limiter (`zzz-326`, aimed at the stutter in #31) is included but **off by default** (`zzz-330`).
  On an RX 580 with the GPU saturated (3x, AF 16x) a limit of 2 made frame pacing much worse (p95 33 -> 51-74 ms);
  a limit of 3 was neutral. Default behaviour is that of v0.6.0-rc.1. Testers can try `GOW_GS_FRAME_LATENCY=2` or `3`.
  See [GPU-06](docs/research/performance-experiments.md).

### Movies
- Fixed movies flashing an old gameplay frame (or black) between their pictures, seen in the opening movie (`zzz-331`).
  The movie player draws into one buffer and alternates the displayed buffer with a second one that still held the last
  gameplay picture. In the opening movie this went from 11-23 % of screen captures showing the old frame to none; gameplay
  is unaffected (the rule never applied there, even at 45 FPS). See
  [the investigation](docs/research/movie-flash-2026-11-13.md).

### Diagnostics
- Missing-target reports also print the saved registers `$s2`-`$s7` and the words they point to (`zzz-327`), which
  identifies the null object behind the freeze after the Cronos climb (#33) in the next report. The freeze itself is
  not fixed yet.
- Test-only fixed 1/60 s step for reproducible scripted benchmarks (`zzz-328`); never active in normal play.

## [v0.6.0-rc.1] — pre-release for testing

Includes fix-chore and the main changes since v0.5.0. v0.5.0 remains the stable release; this is an opt-in test build.

### Player features
- Windows setup lets new and existing installations choose the game build directory. Relocation requires a rebuild;
  existing discs, saves, settings and the old build are retained. No matching pre-trained PGO profile is bundled.
- Redesigned in-game menu with keyboard, mouse and controller navigation, gameplay pause, live graphics/audio,
  control bindings, accessibility, cheats and active texture-pack listing. Supported settings persist and synchronize
  with the launcher. Startup notice lists active accessibility options; RatOne branding and spray intro are included.
- Mods and texture-pack management share the launcher interface. Packs remain user supplied.
- Automatic backups after successful USA altar saves, with history and in-game notification. Individual active-card
  slots show native location names, thumbnails and local timestamps in the launcher and menu. Select a slot to load
  via the game's normal loader, including during gameplay after confirmation. These are not savestates; full-card
  historical restore still requires closing the game. See [save behavior](docs/saves.md).
- Button prompts follow the last active device and remappings, with Xbox, PlayStation and keyboard/mouse styles.
  Recognized original buttons are suppressed; enemy world badges retain the game's animation. Keyboard stick QTE
  guides show mapped direction sequences. Optional automatic button QTEs and held-button assistance are available.

### Fixes and performance
- Fixed distorted Spartan-style CTRL: shared Greek R2 artwork is replaced only inside a verified live button
  instance. Reproduced and validated on a private copy of slot 3. Linux GS/VU1 diagnostic headers now compile
  with native Clang; Windows setup file properties use the same version as its build definition.
- Fixed the USA movie drained-EOF loop reported at Cliffs of Madness (#30), retaining decoded final pictures;
  corrected movie-tail handling, audio reset phase and bounded audio diagnostics.
- Fixed minimized-window shutdown and collector summaries left running after child exit. Added queue/presentation
  attribution; this does not claim to eliminate every stutter from issue #31.
- VU1 generator processes microcode images one at a time, reducing local memory pressure with identical generated output.
- Default VU1 chain specialization removes diagnostic checks from ordinary execution while retaining semantics.
  Local Athn01D three-pair medians: 59.766 → 59.966 guest FPS, p99 19.490 → 18.537 ms. Isolated task time improved
  7.66%; that is not an FPS percentage. See [method and limitations](docs/research/vu1-chain-2026-11-09.md).
- Shared presentation acquisition avoids one publisher mutex wait. Shader failure handling, texture bounds,
  ring ownership, primitive submission and frame-order fixes accompany the renderer integration below.

### Distribution and remaining limits
- Windows setup and experimental Linux x86-64 `.run`/archive candidates. Linux uses host Qt/QML libraries,
  requires a compatible environment and is not a universally portable Steam Deck build. See [Linux guide](docs/linux.md).
- No ISO, extracted game assets, memory cards or precompiled copyrighted game code is distributed.
- Full campaign, sustained combat on all hardware, physical Sony controllers and world-badge depth occlusion remain
  acceptance gaps. Aphrodite's reported missing dialogue is unproven; asset/mixer checks do not establish a fix.
- Stable 60 FPS on all PCs is a target, not a release guarantee. Experimental logic-rate and renderer switches are
  not universal performance improvements. Candidate validation is recorded in [the release checklist](docs/research/release-0.6.0-candidate.md).

### Renderer integration carried since v0.5.0

- **PR 26 integrated onto fix-chore.** The contributor's renderer work is in: one persistent vertex ring, primitive
  runs, texture-reuse by content hash, one depth buffer per ZBP, cached GL prelude, warm CLUT shader, frame-pacing
  percentiles and spike attribution. Also in: internal resolution below 1x (0.25-4) for integrated GPUs, effect
  switches (shadows, additive effects, texture filtering), a Mods tab with VFS overrides and declarative data
  patches, and a software rasterizer fallback for GPUs without OpenGL 4.3. Every fix-chore fix and test is kept.
- Fixed while integrating: single primitives were dropped by the threaded queue when grouping was off. The
  vertex ring now uses per-segment fences instead of a full GPU wait on every wrap. `glBufferStorage` is used
  below OpenGL 4.4 only when the driver lists it. The installer manifest is back (no elevation prompt), and so is
  the `.gitattributes` rule that keeps runtime patches applying on Windows clones.
- Performance on weaker PCs is not yet measured on this branch: see `docs/research/pr26-integration.md`.

## [v0.5.0] — 2026-11-02

### Added
- **Experimental Linux distribution:** native launcher and diagnostic helper, Linux build pipeline, verified source/binary
  archive and self-extracting `.run` installer. Intended for testing on Linux devices and Steam Deck; compiled and
  smoke-tested in Ubuntu 26.04 WSL. Uses host libraries; SteamOS, real Deck hardware and performance are not validated.
- **CHD import for recompilation:** supported CHDv5 images are verified and converted to a cached ISO, then pass the
  same executable compatibility checks as ISO imports. Extra disk space is required; source images remain read-only.
- **Regions:** USA and the tested USA-identical Asia disc; isolated Japan `SLPM_670.10` and Europe `SCES_531.33`
  Windows builds validated through opening ship gameplay. The launcher selects matching registered builds and keeps
  their saves separate. A fresh installation does not automatically compile foreign-code discs with the USA pipeline.
  Other serials and whole-campaign compatibility remain unvalidated; demos are excluded.
- **Accessibility page:** texture packs and input assistance now have a dedicated page. Existing PCSX2-format PNG
  replacements can customize the visuals; no packs are distributed with the project.
- **Keyboard stick assistance:** optional held shake/rotate binding (G by default), with release/focus-loss handling
  and no changes to connected gamepads. Shoulder-button repetition (L1/L2/R1/R2) is individually opt-in.
- **ISO mods:** verified xdelta/VCDIFF application to a separate output image, preserving the source disc.

### Fixed
- **Cutscene audio:** stale packets from a previous movie no longer corrupt the next movie's channel alignment and
  PT-BR voices (patch 54). The measured PT-BR scene now matches its source audio within rounding error; USA remains
  unchanged. Earlier disappearing-sound-effect fixes are retained. This does not certify every movie or transition.
- **Held keyboard chords:** Ctrl + Space and Ctrl + WASD retain their gameplay buttons; default evade moves to Alt +
  WASD. QTE repetition pauses during chords to avoid interrupting held actions (patches 57–58).
- **Linux startup:** prepared discs include lowercase IOP-module aliases for case-sensitive paths. The diagnostic
  helper now records Linux process CPU/RSS, with platform-correct crash tests.

### Build and performance
- Native IOP modules compile in parallel parts (patch 55). On the recorded Xeon/Clang build, the longest unit fell
  from **126.5 s to 24.5 s**; this is not the complete installation time or a cross-machine benchmark.
- Further compilation-time reductions and better performance on slower PCs remain goals toward v1.0.0, without a
  promised date or duration. Existing VU1/PGO improvements remain; recorded results do not establish locked 60 FPS
  on every mid-range computer or across the full game. See [hardware evidence](docs/tested-hardware.md).
- The v0.5.0 Windows package ships **without a pre-trained PGO profile**; local training remains available.

### Graphics and remaining limits
- Existing internal scaling, sharp filtering, FXAA, anisotropic filtering and borderless external-tool mode remain
  available. **DLSS, including DLSS 5, is not integrated**; external scaling/frame generation is not native DLSS support.
- Transparent enemies were reproduced with a replacement pack; pack data versus renderer handling has not been
  isolated. No blanket alpha correction is included.
- Linux tests reached initial gameplay with software rendering in WSL. Steam Input, suspend/resume, hardware GPU
  performance and Japan/Europe Linux builds still need validation. See [Linux instructions](docs/linux.md) and
  [disc compatibility](docs/disc-import.md).

## [v0.4.0] — 2026-10-31

Changes since v0.3.1 on the development branch.

### Added
- **Fast VU mode** (off by default; Launcher → Performance): VU1 code without the exact float clean-ups, like PCSX2's
  default settings. About 12% less VU1 work in combat; rare small differences from the exact mode (patch 51).
- **Hold instead of mashing** (accessibility, off by default; Launcher → Controls): in the minigames that ask for
  repeated presses, holding the button presses it for you; a short press stays a single press. By
  [ThisIsAkill](https://github.com/ThisIsAkill) (PR #10, patch 53).
- **Launcher in French, German, Italian, Russian and Simplified Chinese**, besides English, Portuguese, Spanish and
  Turkish; the system's language is picked on first start. The in-game F1 menu stays in English for these.
- Releases can ship a profile-guided optimisation (PGO) profile trained on exactly their runtime and patch set.
- `tools/pipeline/patches.ps1`: the runtime patch stack as git commits, to move it to a newer runtime commit.

### Performance
- The VU1 interpreter's debug and recording paths no longer reserve large stack frames on every VU1 program (patch 50).

### Fixed
- **Frozen pink screen when the GPU renderer cannot start** (no OpenGL 4.3: an old GPU, the Microsoft Basic Display
  Adapter, Remote Desktop; or a driver that rejects the shaders): the window now says why and what to do (patch 52).
- **"The term ...\build.ps1 is not recognized" during install**: an antivirus had removed the pipeline script while it
  started. The launcher now says that a file was removed and how to restore it, instead of "Failed (code 1)".
- **Sound effects going silent in long fights**: released voices now fade to zero and free their slot; before, the
  effects voices filled up after about 35 s of combat and hits went silent until an area change (patch 48).
- **Install failed in a folder whose name has a space** (for example `E:\God of War`): the path of the PGO profile was
  cut at the first space and every compile failed ("sccache: Failed to open file for hashing").
- Running the setup over an earlier version now asks for a game rebuild when the runtime patches changed, as launcher
  updates already did.

## [v0.3.1] — 2026-10-31

Formerly v3.0.1. 7 commits since v0.3.0.

### Performance
Measured on the test PC (Xeon E5-2666 v3 + RX 580, full screen, save route, alternating pairs).
- Leaner generated VU1 code, bit-identical results: average about 51 → **57 fps**, heavy combat about 51 → **55 fps**
  (patch 43).
- Lock-free hand-off to the GS stage thread: about 150,000 fewer thread wake-ups per second, combat +3.5% (patch 44).
- VU1 decode on demand and one copy less per GIF packet, from the
  [gow-native-pc-optimization](https://github.com/huanghuangdaquan-beep/gow-native-pc-optimization) fork (patches 45–46).

### Added
- The installer checks for SSE4.2 and tells unsupported CPUs before installing.
- CPUs with 4 threads or fewer install without the profile-guided pass (it can be run later) and with one more
  compiler in parallel.

### Fixed
- GPUs whose drivers stop at OpenGL 4.3–4.5 (Intel HD 4000–4600 and others) use the GPU renderer instead of the much
  slower software one (patch 47).
- Dependencies are downloaded once and without history: the profile-guided step no longer downloads ~1 GB again.
- The "rebuild" notice clears only after a rebuild with the new runtime; after an update the launcher recommends
  training the profile again.

## [v0.3.0] — 2026-10-31

Formerly v3.0.0. 28 commits since v0.2.0.

### Performance
Measured on the test PC (full screen 1080p, 2× internal resolution).
- Heaviest ship combat about 40 → **50–53 fps**; ship average about 55 fps.
- **Early FINISH**: the game prepares the next frame while the VU1 draws the current one, about +20% in heavy combat.
- **Profile-guided optimisation is part of the install** (stage `tune`): about +5% average, +13% in combat.
- **VU1 x86-64 JIT**: areas without recorded VU1 code are compiled while playing (patches 18, 27).
- Leaner JAL/JALR calls and memory fast paths (patches 28–35).

### Added
- In-game **F1 menu**: live Graphics tab, Cheats tab (F2), Save tab; runtime debugger moved to Ctrl+F1.
- Launcher: graphics profiles (Performance / Balanced / Quality), borderless fullscreen, an "Upscaling and tools"
  section, Saves page, PGO status, session reports for bug reports.
- First-play preparation + PGO in about 24 minutes instead of about 63; `build.cmd -Stages verify` regression tests.

### Fixed
- Memory/DMA correctness, SPU2 audio mix overflow, standalone game launch, widescreen movie placeholders.
- FXAA no longer turns off the sharp scaler; settings that did nothing were removed.

## [v0.2.0] — 2026-10-25

Formerly v2.0.0 (its launcher reports 2.1.0). 15 commits since v0.1.0.

### Added
- **clang (llvm-mingw) is the compiler**: the game builds in about 15 minutes instead of 45–60 with MSVC; Visual
  Studio, Git and Python are no longer needed.
- **Install in three steps**: portable tools → ISO → install. New loading screen with one progress bar and time left.
- Controller remapping button by button (patch 14), 32:9 aspect ratio.
- **Launcher updates from GitHub Releases.**
- Optional profile-guided optimisation (Install → More options).

### Performance
- Scripted ship combat: minimum 51.4 → 54.6 fps, average 56.5 → 58.4 fps (clang + PGO).
- VU1 without needless float clean-ups and with chained blocks (patches 10, 13); sound mixed in batches; the IOP heap
  reuses freed memory (patch 11).

### Fixed
- Builds from USB / exFAT drives; the Python check; the launcher reads the game path again after a build.

## [v0.1.0] — 2026-10-25

Formerly v1.0.0. 12 commits: the first release.

### Added
- Build pipeline with per-game profiles, one-file setup (no administrator rights) with uninstaller, launcher.
- Fan translations accepted by language, with a language choice when several ISOs are added.
- Anisotropic filtering option.

### Performance
- Scripted ship combat with the default settings: average about 48 → 58 fps (every VU1 block as native code, cheap VU1
  recording, faster sound mixing, "register locals" code generation, patches 08–09).

### Fixed
- Image fixes at higher internal resolutions from the God of War II native port; MOVZ/MOVN translation; runtime memory
  out of the game's heap; FMV audio streams; faster memory card saves; runtime patches keep their line endings.

[v0.5.0]: https://github.com/Vnnc1/gow-native-pc/releases/tag/v0.5.0
[v0.4.0]: https://github.com/Vnnc1/gow-native-pc/releases/tag/v0.4.0
[v0.3.1]: https://github.com/Vnnc1/gow-native-pc/releases/tag/v0.3.1
[v0.3.0]: https://github.com/Vnnc1/gow-native-pc/releases/tag/v0.3.0
[v0.2.0]: https://github.com/Vnnc1/gow-native-pc/releases/tag/v0.2.0
[v0.1.0]: https://github.com/Vnnc1/gow-native-pc/releases/tag/v0.1.0
