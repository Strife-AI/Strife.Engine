# Strife.Engine
X2D: Strife's custom game engine!

Build:
1. git clone --recurse-submodules -j8 git@github.com:Strife-AI/Strife.Engine.git
2. vcpkg install ms-gsl glm slikenet --triplet x64-windows
3. Copy libtorch-debug and libtorch-release to the Strife.Engine directory
4. Open the CMakeLists in VS and build the project
5. Open the content manager. File -> Open project -> Strife.Engine\examples\sample\sample.json. File -> Preferences. Engine dir: Strife.Engine\out\build\x64-Debug\Debug. X2D Executable: sample.exe
6. Build the content and launch the game
