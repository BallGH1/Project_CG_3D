# OpenGL Starter

[![Build](https://github.com/momotheoxy/OpenGLStarter/actions/workflows/build.yml/badge.svg)](https://github.com/momotheoxy/OpenGLStarter/actions/workflows/build.yml)

A small **C++17 + OpenGL 3.3** starter project for students using **Windows, macOS, or Linux**.

The project uses:

- **GLFW** — window creation, OpenGL context, input and events
- **GLEW** — OpenGL function loading
- **GLM** — vectors, matrices and other graphics mathematics
- **CMake** — cross-platform project configuration and build
- **vcpkg** — automatic C++ dependency installation
- **VS Code** — recommended editor/IDE for the lab

After a successful build, the program opens a window and draws a red triangle.

> You do **not** need to download GLEW, GLFW or GLM manually. vcpkg installs them during CMake configuration.

---

## 1. Install VS Code extensions

Open this repository in VS Code and install the recommended extensions when prompted:

- **C/C++** — Microsoft
- **CMake Tools** — Microsoft

You can also open **Extensions → Recommended** and install them there.

---

# Windows

## 2A. Install the compiler and tools

### Recommended: Visual Studio Community 2026

Install **Visual Studio Community 2026** and select the workload:

**Desktop development with C++**

Download Visual Studio from:

https://visualstudio.microsoft.com/downloads/

### Important: CMake 4.2+ is required for Visual Studio 2026

CMake added the **Visual Studio 18 2026** generator in **CMake 4.2**. If your CMake is older, the VS2026 preset cannot work.

Open **PowerShell** and install/update Git and CMake. **WinGet is the easiest option, but it is not required for this project.**

### Option 1: Install with WinGet

```powershell
winget install --id Git.Git -e
winget install --id Kitware.CMake -e
```

If PowerShell says `winget` is not recognized, see **Windows: `winget` is not recognized** in the Troubleshooting section below. On managed/lab computers, you can also skip WinGet and install Git and CMake manually.

### Option 2: Install manually

- Git for Windows: https://git-scm.com/download/win
- CMake: https://cmake.org/download/

When installing CMake manually, allow the installer to add CMake to `PATH` if that option is offered.

Check the installations:

```powershell
git --version
cmake --version
```

For the **Visual Studio 2026** preset, use **CMake 4.2 or newer**.

> Still using Visual Studio 2022? That is supported too. Choose the **Windows MSVC (Visual Studio 2022)** preset instead.

## 2B. Install vcpkg

In PowerShell:

```powershell
git clone https://github.com/microsoft/vcpkg $env:USERPROFILE\vcpkg
& $env:USERPROFILE\vcpkg\bootstrap-vcpkg.bat
```

The starter automatically looks for vcpkg in `%USERPROFILE%\vcpkg`, so no environment variable is required when you use this recommended location.

Verify the toolchain file exists:

```powershell
Test-Path "$env:USERPROFILE\vcpkg\scripts\buildsystems\vcpkg.cmake"
```

It should print `True`. If you installed vcpkg somewhere else, set `VCPKG_ROOT` to that folder before configuring CMake.

---

# macOS

## 2A. Install command-line tools

```bash
xcode-select --install
```

## 2B. Install Homebrew tools

If Homebrew is not installed:

```bash
/bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"
```

Then:

```bash
brew install git cmake ninja pkg-config
```

## 2C. Install vcpkg

```bash
git clone https://github.com/microsoft/vcpkg ~/vcpkg
~/vcpkg/bootstrap-vcpkg.sh
```

The starter automatically looks for vcpkg in `~/vcpkg`, so no environment variable is required when you use this recommended location.

Verify the toolchain file exists:

```bash
ls ~/vcpkg/scripts/buildsystems/vcpkg.cmake
```

If you installed vcpkg somewhere else, set `VCPKG_ROOT` to that folder before configuring CMake.

### Apple Silicon or Intel?

Check with:

```bash
uname -m
```

- `arm64` → choose **macOS Apple Silicon (arm64)**
- `x86_64` → choose **macOS Intel (x64)**

The project requests OpenGL **3.3 core**, which is supported by macOS. Apple has deprecated OpenGL, but macOS still provides OpenGL up to 4.1.

---

# Linux (Ubuntu / Debian)

## 2A. Install compiler, build tools and GLFW system dependencies

```bash
sudo apt update
sudo apt install -y \
  build-essential \
  git \
  cmake \
  ninja-build \
  pkg-config \
  autoconf \
  autoconf-archive \
  automake \
  libtool \
  libxinerama-dev \
  libxcursor-dev \
  xorg-dev \
  libglu1-mesa-dev
```

The X11/Mesa packages are required because the vcpkg GLFW port still relies on several Linux system libraries.

## 2B. Install vcpkg

```bash
git clone https://github.com/microsoft/vcpkg ~/vcpkg
~/vcpkg/bootstrap-vcpkg.sh
```

The starter automatically looks for vcpkg in `~/vcpkg`, so no environment variable is required when you use this recommended location.

Verify the toolchain file exists:

```bash
ls ~/vcpkg/scripts/buildsystems/vcpkg.cmake
```

If you installed vcpkg somewhere else, set `VCPKG_ROOT` to that folder before configuring CMake.

---

# 3. Get the project

```bash
git clone https://github.com/momotheoxy/OpenGLStarter.git
cd OpenGLStarter
```

Open **this folder itself** in VS Code — the folder containing `CMakeLists.txt`, `CMakePresets.json` and `vcpkg.json`.

---

# 4. Configure

In VS Code:

1. Press **Ctrl+Shift+P** (Windows/Linux) or **Cmd+Shift+P** (macOS).
2. Run **CMake: Select Configure Preset**.
3. Select the preset matching your computer:

| Platform | Preset |
|---|---|
| Windows + Visual Studio 2026 | `Windows MSVC (Visual Studio 2026)` |
| Windows + Visual Studio 2022 | `Windows MSVC (Visual Studio 2022)` |
| Apple Silicon Mac | `macOS Apple Silicon (arm64)` |
| Intel Mac | `macOS Intel (x64)` |
| 64-bit Linux | `Linux x64` |

CMake will configure the project and vcpkg will automatically download/build the required C++ libraries on the first run. The project finds vcpkg automatically when it is installed in the recommended location, so VS Code does not need to inherit a `VCPKG_ROOT` variable.

The **first configuration/build can take several minutes**. Later builds are much faster.

---

# 5. Build and run

## Build

Press:

**F7**

or run:

**CMake: Build**

from the Command Palette.

## Run / debug

After the build succeeds, press:

**F5**

Choose the matching debug configuration if VS Code asks:

- Windows → `Debug (Windows - MSVC)`
- macOS → `Debug (macOS - LLDB)`
- Linux → `Debug (Linux - GDB)`

You should see a black window with a **red triangle**.

---

# Folder structure

```text
OpenGLStarter/
├── .vscode/
│   ├── extensions.json
│   ├── launch.json
│   └── settings.json
├── Shaders/
│   ├── shader.frag
│   └── shader.vert
├── src/
│   ├── Libs/
│   │   ├── Mesh.cpp
│   │   ├── Mesh.h
│   │   ├── Shader.cpp
│   │   ├── Shader.h
│   │   ├── Window.cpp
│   │   └── Window.h
│   ├── ProjectPaths.h.in
│   └── main.cpp
├── CMakeLists.txt
├── CMakePresets.json
├── vcpkg.json
└── README.md
```

Each configure preset has its own build directory under `build/`, so changing from one preset to another does not reuse an incompatible CMake cache.

---

# Troubleshooting

## Windows: `winget` is not recognized

WinGet is included with Microsoft's **App Installer** package. First check whether App Installer is registered for your Windows user:

```powershell
Get-AppxPackage Microsoft.DesktopAppInstaller
```

If this prints package information but `winget --version` is still not recognized, repair WinGet from PowerShell using Microsoft's recommended commands:

```powershell
Install-PackageProvider -Name NuGet -Force | Out-Null
Install-Module -Name Microsoft.WinGet.Client -Force -Repository PSGallery | Out-Null
Repair-WinGetPackageManager -Force -Latest
```

Close PowerShell completely, open a new PowerShell window, and verify:

```powershell
winget --version
```

If App Installer is not installed, install/update **App Installer** from Microsoft Store. Microsoft also provides a manual `.msixbundle` installation route for computers where Store installation is unavailable.

Microsoft troubleshooting: https://learn.microsoft.com/windows/package-manager/winget/troubleshooting

> WinGet is only a convenience for installing Git and CMake. If it remains unavailable on a managed university/lab computer, install Git and CMake manually and continue with the lab.

---

## `Bad CMake executable: ""` on macOS

First verify that CMake itself is installed:

```bash
which cmake
cmake --version
```

With Homebrew on Apple Silicon, `which cmake` will normally print `/opt/homebrew/bin/cmake`. If these commands work but CMake Tools still reports an empty executable, open **Preferences: Open User Settings (JSON)** and remove an empty setting such as:

```json
"cmake.cmakePath": ""
```

If necessary, set it explicitly to the path printed by `which cmake`, then fully quit and reopen VS Code.

---

## `Could not find toolchain file ... vcpkg.cmake`

The starter now tries, in order:

1. an explicitly supplied `CMAKE_TOOLCHAIN_FILE`;
2. `VCPKG_ROOT`, if you use a custom vcpkg location;
3. `%USERPROFILE%\vcpkg` on Windows;
4. `~/vcpkg` on macOS/Linux.

For the recommended installation, verify the file exists:

### Windows

```powershell
Test-Path "$env:USERPROFILE\vcpkg\scripts\buildsystems\vcpkg.cmake"
```

### macOS / Linux

```bash
ls ~/vcpkg/scripts/buildsystems/vcpkg.cmake
```

If CMake previously cached a broken path such as `/scripts/buildsystems/vcpkg.cmake`, run **CMake: Delete Cache and Reconfigure**. If that still uses the old cache on macOS, remove only the current preset build directory and configure again:

```bash
rm -rf build/mac-arm64
cmake --preset mac-arm64
```

For an Intel Mac, use `build/mac-intel` and `mac-intel` instead.

---

## Visual Studio 2026 preset is missing or unsupported

Run:

```powershell
cmake --version
```

Visual Studio 2026 support requires **CMake 4.2+**.

Also confirm that **Visual Studio Community 2026 → Desktop development with C++** is installed.

---

## CMake selected MinGW on Windows

This starter intentionally uses **MSVC**, not MinGW.

Run:

**CMake: Select Configure Preset**

and select one of the Windows MSVC presets. If necessary, use:

**CMake: Delete Cache and Reconfigure**

---

## Linux: GLFW fails while configuring

Make sure the Linux system dependencies were installed:

```bash
sudo apt install -y \
  pkg-config autoconf autoconf-archive automake libtool \
  libxinerama-dev libxcursor-dev xorg-dev libglu1-mesa-dev
```

Then delete the failed build directory or run **CMake: Delete Cache and Reconfigure**.

---

## Shader files cannot be found

The starter does **not** depend on the program's current working directory for shader files. During CMake configuration, `src/ProjectPaths.h.in` is used to generate a header containing the absolute path to this clone's `Shaders` folder. The program then opens `shader.vert` and `shader.frag` from that folder using `std::filesystem`.

Students should edit only the shader files in the repository's top-level `Shaders/` folder. There is no second copied shader folder beside the executable.

If the repository is moved or renamed after CMake has already been configured, run **CMake: Delete Cache and Reconfigure** so the generated path is refreshed. If a shader still cannot be opened, the program prints the exact path it attempted to read.

---

## Project is inside OneDrive or a path with unusual characters

If Windows produces unexplained build/file-lock problems, move the repository to a short local path such as:

```text
C:\Dev\OpenGLStarter
```

This is not an OpenGL requirement, but it avoids several common beginner setup problems.

---

# Dependency versions

The dependencies are declared in `vcpkg.json`, so students normally do not need to manage versions manually.

As of **August 2026**, upstream releases include:

- GLEW **2.3.1**
- GLFW **3.5.1**
- GLM **1.0.3**

The exact version installed by vcpkg depends on the vcpkg registry revision being used.

---

# Notes for the Computer Graphics lab

This repository intentionally keeps the starter code small. The first program contains three helper classes:

- `Window` — creates and owns the GLFW/OpenGL window and context
- `Shader` — reads, compiles and links GLSL shaders
- `Mesh` — creates and renders a VAO/VBO/index buffer mesh

The starter shader renders one red triangle. Later lab work can extend this code with transformations, uniforms, vertex colours, indexed drawing, textures, cameras and lighting.

---

# Credits

This project uses open-source software including **GLEW**, **GLFW**, **GLM**, **CMake** and **vcpkg**.

Created for educational use in Computer Graphics teaching.

---

# Automatic build check (GitHub Actions)

Every push to `main` and every pull request is automatically **compiled** on:

- Windows Server 2025 with **Visual Studio 2026**
- macOS 26 on **Apple Silicon (arm64)**
- Ubuntu 24.04 on **x64 Linux**

The workflow is stored in `.github/workflows/build.yml`. It builds the program but does not try to open the OpenGL window, because GitHub's build runners are used only to verify that the code and dependencies compile successfully.

If the badge at the top of this README says **passing**, the current `main` branch has passed these cross-platform build checks.
