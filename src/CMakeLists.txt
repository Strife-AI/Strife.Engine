cmake_minimum_required(VERSION 3.2 FATAL_ERROR)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${TORCH_CXX_FLAGS}")

find_package(SDL2 REQUIRED)
find_package(SDL2-image REQUIRED)
find_package(unofficial-box2d CONFIG REQUIRED)

add_library(Strife.Engine STATIC
        Engine.cpp
        Math/Rectangle.cpp
        Math/Vector2.cpp
        Math/Polygon.cpp
        Memory/Crc32.cpp
        Memory/Grid.cpp
        Memory/ResourceManager.cpp
        Physics/Physics.cpp
        Physics/ColliderHandle.cpp
        Renderer/Renderer.cpp
        Renderer/SdlManager.cpp
        Renderer/Sprite.cpp
        Renderer/SpriteAtlas.cpp
        Renderer/TilemapRenderer.cpp
        Scene/Entity.cpp
        Scene/Scene.cpp
        Scene/SceneManager.cpp
        Scene/IGame.cpp
        Scene/TilemapEntity.cpp
        System/BinaryStreamReader.cpp
        System/BinaryStreamWriter.cpp
        System/FileSystem.cpp
        System/Input.cpp
        System/Logger.cpp
        System/ResourceFileReader.cpp
        System/ResourceFileWriter.cpp
        System/TileMapSerialization.cpp
        Memory/Fsm.hpp
        Memory/EnumDictionary.hpp
        Memory/SpinLock.cpp
        Memory/SpinLock.hpp
        Tools/ConsoleVar.cpp
        Tools/ConsoleVar.hpp
        Tools/ConsoleCmd.cpp
        Tools/ConsoleValueParsing.hpp
        Tools/Console.cpp
        #../../cpp.hint

        ../imgui/imgui_widgets.cpp
        ../imgui/imgui.cpp
        ../imgui/imgui_draw.cpp
        ../imgui/imgui_demo.cpp
        ../imgui/examples/imgui_impl_opengl3.cpp
        ../imgui/examples/imgui_impl_sdl.cpp

        Renderer/Lighting.hpp
        Renderer/Lighting.cpp
        Renderer/FrameBuffer.hpp
        Renderer/FrameBuffer.cpp
        Renderer/GL/gl3w.c
        Tools/Console.hpp
        Tools/ConsoleValueParsing.cpp
        Renderer/SpriteFont.cpp
        Renderer/SpriteFont.hpp
        Renderer/Color.hpp 

        Tools/MetricsManager.cpp
        Tools/MetricsManager.hpp
        Tools/PlotManager.cpp
        Tools/PlotManager.hpp
        Memory/Singleton.hpp
        Renderer/GL/Shader.cpp
        Renderer/GL/Shader.hpp
        Renderer/LightAnimations.cpp
        Renderer/Camera.cpp
        Renderer/Camera.hpp
        Math/Vector3.hpp
        Math/Vector4.hpp
        Renderer/GL/LineRenderer.cpp
        Renderer/GL/LineRenderer.hpp
        Renderer/RenderVertex.hpp
        Renderer/GL/SpriteBatcher.cpp
        Renderer/GL/SpriteBatcher.hpp
        Renderer/Texture.cpp
        Renderer/Texture.hpp
        UI/UiElement.cpp
        System/SpriteAtlasSerialization.hpp
        System/SpriteAtlasSerialization.cpp
        System/Serialization.hpp
        System/Serialization.cpp
        System/UiSerialization.cpp
        System/UiSerialization.hpp
        Physics/NewPhysics.cpp

        Scene/Timer.cpp
        Memory/BehaviorTree.cpp
        Tools/RawFile.hpp
        Memory/String.hpp
        Scene/CameraFollower.hpp
        Scene/CameraFollower.cpp
        Memory/BlockAllocator.hpp
        Memory/BlockAllocator.cpp
        Memory/StackAllocator.hpp
        Memory/StackAllocator.cpp
        Memory/FixedLengthString.hpp
        Math/Polygon.hpp
        Scene/AmbientLightEntity.hpp
        Scene/AmbientLightEntity.cpp
        Scene/MapSegment.cpp
        Memory/Lazy.hpp
        Memory/EngineEvent.hpp
        UI/UI.cpp
        Sound/SoundManager.cpp
        Sound/SoundManager.hpp
        Sound/SoundEffect.hpp
        Renderer/ThreeSlice.hpp
        Renderer/ParticleRenderer.hpp
        Renderer/ParticleRenderer.cpp
        UI/UiDictionary.hpp
        UI/UiBuilder.cpp
        Memory/Dictionary.hpp
        Math/BitMath.cpp
        Sound/SoundEffect.cpp
        Memory/ResourcePool.hpp
        Renderer/ParticleSystem.hpp
        Renderer/ParticleSystem.cpp
        Math/Random.cpp
        Math/Random.hpp

 		Memory/Cipher.hpp
 		Memory/Cipher.cpp
 		Scene/EntityComponent.hpp
 		Memory/Flags.hpp
 		Components/SpriteComponent.hpp
 		Components/SpriteComponent.cpp
 		Scene/EntityComponent.cpp
 		Components/RigidBodyComponent.hpp
 		Components/RigidBodyComponent.cpp

        Memory/Util.hpp        
)

message("SDL DIRS: ${SDL2_INCLUDE_DIRS}")

# Workaround because OALWrapper can't find SDL path
find_path(SDL_INCLUDE_DIR SDL.h PATH_SUFFIXES SDL2)

target_include_directories(Strife.Engine
        PUBLIC
            $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
            $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../OALWrapper/include>
            $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../imgui>
        PRIVATE
            ${SDL2IMAGE_INCLUDE_DIRS}
            ../imgui/examples
            Renderer)

target_link_libraries(Strife.Engine PUBLIC
    SDL2::SDL2
    SDL2::SDL2main
    SDL2::SDL2_image
    unofficial::box2d::box2d
    OALWrapper
    )

target_compile_definitions(Strife.Engine PUBLIC IMGUI_IMPL_OPENGL_LOADER_GL3W)

find_package(Microsoft.GSL CONFIG REQUIRED)
target_link_libraries(Strife.Engine PRIVATE Microsoft.GSL::GSL)


set_property(TARGET Strife.Engine PROPERTY CXX_STANDARD 17)

