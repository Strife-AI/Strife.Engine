![StrifeEngineLogo](https://user-images.githubusercontent.com/7697514/112564487-bcba8400-8db1-11eb-812d-a25270b8385e.png)

The Strife Engine is originally an internal custom engine developed by Strife AI LLC and used in the development of games such as [Human-Like](https://store.steampowered.com/app/1400190/HumanLike/). In order to support further work on machine learning for game development, we have provided source code for our engine under a [modified UIUC/NCSA license](https://github.com/Strife-AI/Strife.Engine/blob/master/LICENSE.txt).

## Starter projects
To make getting started easier, feel free to clone and use these starter projects.
* [Singleplayer Demo](https://github.com/Strife-AI/Strife.SingleplayerDemo)
* [Multiplayer Demo](https://github.com/Strife-AI/Strife.MultiplayerDemo)

## Getting started
These instructions are written using the CLion IDE, but utilizes CMake as its build system, so any workflow using CMAKE will suffice.

### Prerequisites
There are a few things before getting started:
* C++ compiler and build tools: The engine builds and runs on MSVC, Gcc, and Clang (For Windows, Linux, and Mac respectively). Using CMake as the build system.
    * For Windows: use the Visual Studio C++ build tools.
    * For Linux: Gcc ships with virtually all distributions.
    * For macOS: Install the Xcode command line tools using:
```shell
  xcode-select --install
```
* Cmake
* Configured git client
* [A downloaded copy of LibTorch](https://pytorch.org/get-started/locally/)
    * The latest stable version of LibTorch (1.7.1) should do the trick
    * Note for Windows: Copies of both the debug and release versions of libtorch are required. 
      You'll also need to setup enviroment variables (See: [Setting Up Enviroment Variables](#setting-up-enviroment-variables-windows-only))
    * Note for macOS: CUDA is not available for Mac. So make sure to select “None” for CUDA version.
* [vcpkg](https://github.com/microsoft/vcpkg), which is used to manage dependencies
* Optional: The current version of the engine utilizes [Tiled](MapEditor.org) as its map Editor and [X2cm](https://github.com/Strife-AI/X2DContentManager/releases/tag/v1.8.15-stable) which manages content.
    * These are being phased out in favor of a first-class editor integration for the engine.

# Initial Setup
### Setting up Vcpkg
Clone Vcpkg:
```shell
git clone git@github.com:microsoft/vcpkg.git
```
Run the bootstrap script for your respective platform
* For Windows:
```powershell
./bootstrap-vcpkg.bat
```
* For Linux:
```shell
./bootstrap-vcpkg.sh
```
* For macOS 10.14 above using the Apple Clang compiler:
```shell
./bootstrap-vcpkg.sh --allowAppleClang
```

Install dependencies:
```shell
./vcpkg install sdl2 sdl2-image box2d nlohmann-json openal-soft libogg libvorbis ms-gsl glm slikenet --triplet {x64-windows, x64-linux, x64-osx}
```
### Setting up enviroment variables (Windows Only)
* If you're on Windows, set up environment variables ([How to access them in Windows 10](https://www.wikihow.com/Create-an-Environment-Variable-in-Windows-10))
named `TORCH_DEBUG_DIR` and `TORCH_RELEASE_DIR` which point to the root 
directory of the debug and release versions of libtorch.
  * Once these are set, restart Windows to ensure the changes take place.

### Setting up Strife Engine
Clone the repo (if you are using the sample projects, then substitute in the name of that repo instead of Strife.Engine):
```shell
git clone --recurse-submodules -j8 git@github.com:Strife-AI/Strife.Engine.git
```

Unzip libtorch and place the unzipped directory in a useful place. Our team typically places this in the root directory of the engine repository.

In CLion, open the project and setup CMake configurations (File → Preferences on Windows/Linux or cmd ⌘ + , on Mac). Set the following in the CMake options:

```cmake
-DCMAKE_TOOLCHAIN_FILE="{path to vcpkg}/scripts/buildsystems/vcpkg.cmake"
-DCMAKE_PREFIX_PATH="{path to libtorch}/share/cmake/"
```

If you need to specify a specific platform or architecture, add the CMake variable before `DCMAKE_TOOLCHAIN_FILE`:

```cmake
-DVCPKG_TARGET_TRIPLET={x64-windows, x64-linux, x64-osx} 
```

*Note: Macs using Apple Silicon do build and work properly via Rosetta 2. But is not working natively at this time. arm64 versions of Windows and Linux have not been tested at this time.*

## Resource Paks / Content Pipeline
Resource Paks are binary archives of data that the engine uses. Resource paks can be built using the Content Pipeline project (ContentPipeline2) within the repo. 
Building content requires a resources list in a json format. For example, here is a resource list from [Strife.MultiplayerDemo](https://github.com/Strife-AI/Strife.MultiplayerDemo/blob/main/MultiplayerDemo.json).

The usage for the content pipline is as follows:
```shell
./ContentPipeline2 -i /path/to/content.json -o /path/to/build/output
```

By default the resource pak will be outputted in the current directory if an output directory is not supplied.
The game configuration will expect the resource pak in the same directory along with the executable.

*Note: The content pipeline is currently in the process of being phased out in favor of an editor and a self contained bundle manager*
