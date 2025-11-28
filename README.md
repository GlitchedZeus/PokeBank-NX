# **DISCLAIMER**
### THIS IS IN ACTIVE DEVELOPMENT AND ONLY A PROTOTYPE AT THIS POINT. I AM NOT RESPONSIBLE IN ANY WAY FOR DAMAGES TO YOUR SWITCH.
# USE AT YOUR OWN RISK.
# **THIS IS NOT A PLACE TO DISCUSS PIRACY OR HOW TO HACK/JAILBREAK/MOD A SWITCH**
---
# **PKSE - Pokemon Save Editor**
PKSE is a homebrew application for editing Pokemon save files on the Nintendo Switch.

## **Features**
- Backup and restore save files.
- Edit Pokemon party, box, and item data. (Currently In development)

---

## **Prerequisites**

### 1. Install Required Tools
Ensure the following tools and dependencies are installed:

#### **1.1. devkitPro**
- Download and install [devkitPro](https://devkitpro.org/wiki/Getting_Started).
- Ensure `Switch Development` is selected during installation.

#### **1.2. zlib installation** (Optional, will implement compressed logic in future versions)
- In the MSys2 shell, run ```pacman -S switch-zlib``` to install the zlib for compression support.

---

### 2. Set Up Environmental Variables
Set the `DEVKITPRO` environment variable to the installation path of devkitPro.

#### On Windows:
```bash
setx DEVKITPRO "C:\devkitPro"
```
#### On macOS/Linux:
Add the following line to your shell configuration file (~/.bashrc or ~/.zshrc):
```bash
export DEVKITPRO=/opt/devkitpro
```

Restart your terminal or run the command to apply the changes.

---

### 3. Run "download_sprites.ps1" powershell script.

See SPRITES_INSTRUCTIONS.md for more details.
You'll need to run the download_sprites.ps1 script to download all of the necessary sprites. If you're on linux, you'll likely need an alternative method or manually download the sprites from https://github.com/PokeAPI/sprites/tree/master/sprites/pokemon

---

### 4. Configure Visual Studio Code

To configure IntelliSense in VS Code:

#### **4.1. Install Extensions**
- C/C++ by Microsoft
- DevkitPro Tools (if available)

#### **4.2. Create a c_cpp_properties.json File**
Create or update the file in .vscode/c_cpp_properties.json with the following content:
```json
{
  "configurations": [
    {
      "name": "Switch",
      "includePath": [
        "${workspaceFolder}/include/**",
        "${workspaceFolder}/src/**",
        "${env:DEVKITPRO}/libnx/include",
        "${env:DEVKITPRO}/portlibs/switch/include", // We should include optional libraries here
        "${env:DEVKITPRO}/devkitA64/aarch64-none-elf/include"
      ],
      "defines": [],
      "compilerPath": "${env:DEVKITPRO}/devkitA64/bin/aarch64-none-elf-g++.exe",
      "cStandard": "c11",
      "cppStandard": "c++20", // This version is necessary
      "intelliSenseMode": "linux-gcc-arm64"
    }
  ],
  "version": 4
}
```

---

## **5. Build the Project**

To build the project, open MSys2 (should have been included with the devkitpro toolset), navigate to the root directory and run:

```bash
make clean && make
```

The output will generate an .nro file in the build directory, which you can deploy to your Nintendo Switch.

---

## **Troubleshooting**

### Common Issues

- **`make` not found**:  
  Ensure `make` is installed and in your `PATH`.

- **Undefined references**:  
  Verify that your `includePath` is correctly configured in `c_cpp_properties.json`.

- **libnx-related errors**:  
  Ensure `libnx` is properly installed and that `DEVKITPRO` is set correctly.

- **Permission issues on Windows**:  
  Run VS Code or your terminal as Administrator if file access errors occur.

---

## **Credits**

- PKHeX Team: Core save editing logic and Pokemon data structures are derived from the PKHeX project. Visit their official repository: https://github.com/kwsch/PKHeX.
- PokeAPI team
- libnx and devkitPro communities for Switch homebrew development tools. Visit their official website: https://devkitpro.org/wiki/Getting_Started.

## **License**

This project is licensed under the [GNU Affero General Public License v3.0](LICENSE) or later. See `LICENSE` for details.

