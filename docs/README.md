<div align="center">

# PlantsVsZombies AndroidTV

**English** | **[简体中文](./README.zh-cn.md)**

[![license](https://img.shields.io/github/license/ZombieYetis/PlantsVsZombies-AndroidTV)][GPL-3.0]
[![Android CI](https://github.com/ZombieYetis/PlantsVsZombies-AndroidTV/actions/workflows/android.yml/badge.svg)](https://github.com/ZombieYetis/PlantsVsZombies-AndroidTV/actions/workflows/android.yml "Android CI")

A mod of _Plants vs. Zombies_ Android TV version.

</div>

## Build

- Ensure the following is installed:
    * Android SDK Platform 34
    * NDK v27.3.13750724 (r27d)
    * CMake v3.20+

- Clone the repository **with its submodules** (they carry dynarmic, SDL, zlib and glad).
    ```sh
    git clone --recursive https://github.com/ZombieYetis/PlantsVsZombies-AndroidTV.git
    cd PlantsVsZombies-AndroidTV
    ```
    > Already cloned without `--recursive`? Run `git submodule update --init --recursive`.
    > To build against dependency copies you already have on disk, configure with
    > `-DPVZTV_DEPS_DIR=<dir>` (see [`third_party/CMakeLists.txt`](/third_party/CMakeLists.txt)).

- Copy assets files to the path `PlantsVsZombies-AndroidTV/app/src/main/assets/`.
    > If you need resource files, please contact the repository author.

- Build with:
    * Android Studio: Click on the build button.
    * Command line: Run the following command:
        ```sh
        ./gradlew assembleDebugV115
        ```

- If release, configure signing using the file `keystore.properties` located in the project root directory (you must
    create this file yourself). The file content format is as follows:
    ```properties
    storePassword=myStorePassword
    keyPassword=mykeyPassword
    keyAlias=myKeyAlias
    storeFile=myStoreFileLocation
    ```

## Desktop player

The same runner that lets the arm64 build execute the game's original 32-bit ARM
libraries also runs them on a PC, through SDL2 instead of the Android system
libraries.

```sh
cmake -S desktop -B build/desktop
cmake --build build/desktop --config Release
```

Run the resulting `pvztv_player` from a directory holding the game's `assets/`
and the guest libraries; see [Architecture](./ARCHITECTURE.md) for what it loads
and why.

## Contributing

### Coding Style (C++)

#### Name Convention

- Functions / types / concepts: `PascalCase`
- Variables: `camelCase`
- Namespaces: `snake_case`
- Macros / constants / enumerators / non-type template parameters: `UPPER_CASE`

#### Format

See [`.clang-format`](/.clang-format).

> It is recommended to format the code using the IDE before each commit.

### Commit

Refer to [Conventional Commits](https://www.conventionalcommits.org/en/v1.0.0/).

## License

The source code for this project is licensed under the [GPL-3.0][GPL-3.0] license.

This project is not associated with or endorsed by Transmension, PopCap or Electronic Arts.

[GPL-3.0]: https://www.gnu.org/licenses/gpl-3.0.html "GPL-3.0"
