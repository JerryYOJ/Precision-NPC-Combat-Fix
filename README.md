# Precision NPC Combat Fix — Skyrim AE cross-build

This fork builds the plugin DLL for **Windows x64** from Ubuntu using Clang’s MSVC-compatible driver. The build target is Skyrim AE, including runtime **1.7.104.0**.

> **Status:** The DLL has been compile- and PE-format-verified. Runtime compatibility, relocation targets, and gameplay in Skyrim 1.7.104.0 still need to be verified in-game.

## What changed

- CommonLibSSE-NG is built from the pinned source submodule at `.buildenv/_deps/CommonLibSSE-NG` (`v11.0.0`); the separate CommonLib v3.8.0 vcpkg port was removed.
- `.buildenv/CMakeLists.txt` includes CommonLib’s plugin helper and builds with AE enabled, SE/VR disabled, and CommonLib tests disabled.
- The `linux-clangcl` CMake preset configures Ninja, vcpkg’s `x64-windows-clangcl` triplet, the CommonLib Linux toolchain, its DirectXTK overlay, and the xwin sysroot.
- MinHook, Boost.Algorithm, Xbyak, the existing PCH, plugin metadata generation, and the original pitch calculation are retained. Automatic game/mod-manager deployment is disabled.
- Unused local RE-header includes that duplicated CommonLib definitions were removed from `CombatAimFix.h`. No aiming-algorithm changes were made.
- Build output is under `build/`; it is ignored by Git.

## Build prerequisites

The tested environment used Ubuntu 24.04, CMake, Ninja, vcpkg, Wine, xwin, and an xwin Windows SDK/MSVC CRT sysroot. The toolchain needs:

- **Clang-CL 19 or newer**, `lld-link`, `llvm-lib`, and `llvm-rc`. The xwin MSVC STL in this environment rejects Clang 18. The successful build used Clang 23.1.0 in CL mode.
- **xwin 0.10.0** and an xwin sysroot, here at `$HOME/xwin-out`.
- **vcpkg**, with `VCPKG_ROOT` pointing to its checkout.
- **llvm-mingw** and its `x86_64-w64-mingw32-clang++` executable, used by the DirectXTK overlay to build the shader compiler workaround.
- **Wine**, used to run the DirectXTK `fxc2` shader compiler workaround.
- The CommonLibSSE-NG v11.0.0 submodule, initialized below.

The build machine used a user-local `clang-cl` symlink to its Clang 23.1.0 executable. On another machine, use a Clang-CL 19+ executable that supports the MSVC target; do not point the build at Clang 18 with this sysroot.

## Dependencies

The project manifest (`.buildenv/vcpkg.json`) directly requests:

| Dependency | Resolved version in the verified build |
|---|---:|
| Boost.Algorithm | 1.89.0 |
| MinHook | 1.3.4 |
| spdlog | 1.16.0 |
| Xbyak | 7.28 |
| DirectXMath | 2025-04-03 |
| DirectXTK | 2026-05-07, overlay revision 2 |
| fmt | 12.1.0 |
| rapidcsv | 8.90 |
| nlohmann-json | 3.12.0, port revision 1 |
| SimpleIni | 4.25 |
| toml11 | 4.4.0 |

The manifest also causes vcpkg to build the packages’ transitive and host dependencies. The vcpkg registry baseline is recorded in `.buildenv/vcpkg-configuration.json`. DirectXTK uses the CommonLib overlay port and its Wine/fxc2 workaround.

## Configure and build

From the repository root, set the tool paths. Adjust these if your installations are elsewhere:

```bash
export VCPKG_ROOT="$HOME/.local/share/vcpkg"
export XWIN_SYSROOT="$HOME/xwin-out"
export LLVM_MINGW_BIN="/opt/llvm-mingw/bin"
export PATH="$HOME/.cargo/bin:$HOME/.local/bin:$PATH"
```

Initialize the pinned submodule if needed:

```bash
git submodule update --init --recursive
```

Check that `clang-cl`, `lld-link`, `llvm-lib`, `llvm-rc`, `wine`, and
`$LLVM_MINGW_BIN/x86_64-w64-mingw32-clang++` are available. Then configure and build:

```bash
cmake --preset linux-clangcl -S .buildenv
cmake --build build/linux-clangcl --parallel "$(nproc)"
```

The first configure runs vcpkg manifest installation and may take a while. The preset targets **Windows x64**, uses the **Release** configuration, and sets `ENABLE_SKYRIM_AE=ON`, `ENABLE_SKYRIM_SE=OFF`, `ENABLE_SKYRIM_VR=OFF`, and `BUILD_TESTS=OFF`.

The verified commands were:

```bash
VCPKG_ROOT="$HOME/.local/share/vcpkg" \
XWIN_SYSROOT="$HOME/xwin-out" \
LLVM_MINGW_BIN=/opt/llvm-mingw/bin \
cmake --preset linux-clangcl -S .buildenv

cmake --build build/linux-clangcl --parallel "$(nproc)"
```

The output is:

```text
build/linux-clangcl/PrecisionNpcAimFix.dll
```

## Verify the build

```bash
file build/linux-clangcl/PrecisionNpcAimFix.dll
llvm-readobj --coff-exports build/linux-clangcl/PrecisionNpcAimFix.dll
llvm-readobj --coff-imports build/linux-clangcl/PrecisionNpcAimFix.dll
```

The verified output was a **PE32+ x86-64 Windows DLL**. Its exports were `SKSEPlugin_Load`, `SKSEPlugin_Query`, and `SKSEPlugin_Version`. It imports the MSVC runtime (`MSVCP140.dll`, `VCRUNTIME140.dll`, and UCRT API-set DLLs) plus Windows system DLLs. Those runtime DLLs are not part of this plugin package; the game’s Windows/Proton environment must provide them.

## Test installation layout

For Mod Organizer 2, install the DLL as a separate mod after the original mod so it overrides only the plugin binary:

```text
Precision NPC Combat Fix - 1.7.104/
└── SKSE/
    └── Plugins/
        └── PrecisionNpcAimFix.dll
```

Keep the original mod’s non-DLL assets enabled. The game also needs SKSE 2.3.1 and the Address Library mapping for runtime 1.7.104.0. Do not copy `CommonLibSSE.lib`, MinHook, or DirectXTK libraries into `SKSE/Plugins`; they are linked into the plugin.

## Compatibility and limitations

A successful cross-build proves the toolchain produced a Windows DLL; it does **not** prove compatibility with the game executable. The source still uses AE relocation IDs `37961` (pitch hook) and `37483` (`IsDead`). The DLL has not been checked against `SkyrimSE.exe` and `versionlib-1-7-104-0.bin`; its target instruction bytes, hook safety, and gameplay behavior remain unverified. CommonLib header declarations and a successful compile are not a substitute for that validation.

The current hook setup also does not check the return codes from `MH_Initialize`, `MH_CreateHook`, or `MH_EnableHook`. This was noted but not changed as part of the build port.

These build commands do not deploy the DLL, modify saves, run the game, create a compatibility ZIP, commit, or push. Runtime compatibility and gameplay still need in-game verification. If you find a bug, please report it in the [GitHub Issues](https://github.com/LinCie/Precision-NPC-Combat-Fix/issues) page, with your Skyrim and SKSE versions, relevant logs, and steps to reproduce.

**AI-use disclaimer:** The build and configuration changes documented here were made using GPT 6.1 Sol. They still need review and in-game verification.
