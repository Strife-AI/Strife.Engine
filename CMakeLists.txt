cmake_minimum_required(VERSION 3.2 FATAL_ERROR)

project(Engine VERSION 1 LANGUAGES C CXX)

find_package(SDL2 REQUIRED)
find_package(SDL2-image REQUIRED)
find_package(unofficial-box2d CONFIG REQUIRED)


# Enable exception unwind semantics
add_compile_options("/EHsc")
add_compile_options("/MP")

add_library(Engine STATIC
        src/Engine.cpp
        src/Math/Rectangle.cpp
        src/Math/Vector2.cpp
        src/Math/Polygon.cpp
        src/Memory/Crc32.cpp
        src/Memory/Grid.cpp
        src/Memory/ResourceManager.cpp
        src/Physics/Physics.cpp
        src/Physics/ColliderHandle.cpp
        src/Renderer/Renderer.cpp
        src/Renderer/SdlManager.cpp
        src/Renderer/Sprite.cpp
        src/Renderer/SpriteAtlas.cpp
        src/Renderer/TilemapRenderer.cpp
        src/Scene/Entity.cpp
        src/Scene/Scene.cpp
        src/Scene/SceneManager.cpp
        src/Scene/IGame.cpp
        src/scene/TilemapEntity.cpp
        src/System/BinaryStreamReader.cpp
        src/System/BinaryStreamWriter.cpp
        src/System/FileSystem.cpp
        src/System/Input.cpp
        src/System/Logger.cpp
        src/System/ResourceFileReader.cpp
        src/System/ResourceFileWriter.cpp
        src/System/TileMapSerialization.cpp
        src/Memory/Fsm.hpp
        src/Memory/EnumDictionary.hpp
        src/Memory/SpinLock.cpp
        src/Memory/SpinLock.hpp
        src/Tools/ConsoleVar.cpp
        src/Tools/ConsoleVar.hpp
        src/Tools/ConsoleCmd.cpp
        src/Tools/ConsoleValueParsing.hpp
        src/Tools/Console.cpp
        #../../cpp.hint

        imgui/imgui_widgets.cpp
        imgui/imgui.cpp
        imgui/imgui_draw.cpp
        imgui/imgui_demo.cpp
        imgui/examples/imgui_impl_opengl3.cpp
        imgui/examples/imgui_impl_sdl.cpp

        src/Renderer/Lighting.hpp
        src/Renderer/Lighting.cpp
        src/Renderer/FrameBuffer.hpp
        src/Renderer/FrameBuffer.cpp
        src/Renderer/GL/gl3w.c
        src/Tools/Console.hpp
        src/Tools/ConsoleValueParsing.cpp
        src/Renderer/SpriteFont.cpp
        src/Renderer/SpriteFont.hpp
        src/Renderer/Color.hpp 

        src/Tools/MetricsManager.cpp
        src/Tools/MetricsManager.hpp
        src/Tools/PlotManager.cpp
        src/Tools/PlotManager.hpp
        src/Memory/Singleton.hpp
        src/Renderer/GL/Shader.cpp
        src/Renderer/GL/Shader.hpp
        src/Renderer/LightAnimations.cpp
        src/Renderer/Camera.cpp
        src/Renderer/Camera.hpp
        src/Math/Vector3.hpp
        src/Math/Vector4.hpp
        src/Renderer/GL/LineRenderer.cpp
        src/Renderer/GL/LineRenderer.hpp
        src/Renderer/RenderVertex.hpp
        src/Renderer/GL/SpriteBatcher.cpp
        src/Renderer/GL/SpriteBatcher.hpp
        src/Renderer/Texture.cpp
        src/Renderer/Texture.hpp
        src/UI/UiElement.cpp
        src/System/SpriteAtlasSerialization.hpp
        src/System/SpriteAtlasSerialization.cpp
        src/System/Serialization.hpp
        src/System/Serialization.cpp
        src/System/UiSerialization.cpp
        src/System/UiSerialization.hpp
        src/Physics/NewPhysics.cpp

        src/Scene/Timer.cpp
        src/Memory/BehaviorTree.cpp
        src/Tools/RawFile.hpp
        src/Memory/String.hpp
        src/Scene/CameraFollower.hpp
        src/Scene/CameraFollower.cpp
        src/Memory/BlockAllocator.hpp
        src/Memory/BlockAllocator.cpp
        src/Memory/StackAllocator.hpp
        src/Memory/StackAllocator.cpp
        src/Memory/FixedLengthString.hpp
        src/Math/Polygon.hpp
        src/Scene/AmbientLightEntity.hpp
        src/Scene/AmbientLightEntity.cpp
        src/Scene/MapSegment.cpp
        src/Memory/Lazy.hpp
        src/Memory/EngineEvent.hpp
        src/UI/UI.cpp
        src/Sound/SoundManager.cpp
        src/Sound/SoundManager.hpp
        src/Sound/SoundEffect.hpp
        src/Renderer/ThreeSlice.hpp
        src/Renderer/ParticleRenderer.hpp
        src/Renderer/ParticleRenderer.cpp
        src/UI/UiDictionary.hpp
        src/UI/UiBuilder.cpp
        src/Memory/Dictionary.hpp
        src/Math/BitMath.cpp
        src/Sound/SoundEffect.cpp
        src/Memory/ResourcePool.hpp
        src/Renderer/ParticleSystem.hpp
        src/Renderer/ParticleSystem.cpp
        src/Math/Random.cpp
        src/Math/Random.hpp

 		src/Memory/Cipher.hpp
 		src/Memory/Cipher.cpp
 		src/Scene/EntityComponent.hpp
 		src/Memory/Flags.hpp
 		src/Components/SpriteComponent.hpp
 		src/Components/SpriteComponent.cpp
 		src/Scene/EntityComponent.cpp
 		src/Components/RigidBodyComponent.hpp
 		src/Components/RigidBodyComponent.cpp

)

message("SDL DIRS: ${SDL2_INCLUDE_DIRS}")

# Workaround because OALWrapper can't find SDL path
find_path(SDL_INCLUDE_DIR SDL.h PATH_SUFFIXES SDL2)

target_include_directories(Engine
        PUBLIC
            $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>
            $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/OALWrapper/include>
            $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../../glm>
            $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/imgui>
        PRIVATE
            ${SDL2IMAGE_INCLUDE_DIRS}
            imgui/examples
            src/Renderer)

target_link_libraries(Engine PUBLIC
    SDL2::SDL2
    SDL2::SDL2main
    SDL2::SDL2_image
    unofficial::box2d::box2d
    OALWrapper)

target_compile_definitions(Engine PUBLIC IMGUI_IMPL_OPENGL_LOADER_GL3W)

find_package(Microsoft.GSL CONFIG REQUIRED)
target_link_libraries(Engine PRIVATE Microsoft.GSL::GSL)


set_property(TARGET Engine PROPERTY CXX_STANDARD 17)

set(CMAKE_CXX_FLAGS "-D_GLIBCXX_USE_CXX11_ABI=0 -DNOGDI -DWIN32")