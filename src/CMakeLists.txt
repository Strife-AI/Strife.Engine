cmake_minimum_required(VERSION 3.2 FATAL_ERROR)

if(NOT DEFINED CMAKE_SUPPRESS_DEVELOPER_WARNINGS)
	set(CMAKE_SUPPRESS_DEVELOPER_WARNINGS 1 CACHE INTERNAL "No dev warnings")
endif()

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${TORCH_CXX_FLAGS}")

find_package(SDL2 REQUIRED)
find_package(SDL2-image REQUIRED)
find_package(box2d CONFIG REQUIRED)
find_package(slikenet CONFIG REQUIRED)
find_package(Ogg REQUIRED)

add_library(Strife.Engine STATIC
        Engine.cpp
        Math/Rectangle.cpp
        
        Math/Polygon.cpp
        Memory/Crc32.cpp

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
        
        
        Tools/ConsoleVar.cpp
        Tools/ConsoleVar.hpp
        Tools/ConsoleCmd.cpp
        Tools/ConsoleValueParsing.hpp
        Tools/Console.cpp
        #../../cpp.hint

		../imgui/imgui_tables.cpp
        ../imgui/imgui_widgets.cpp
        ../imgui/imgui.cpp
        ../imgui/imgui_draw.cpp
        ../imgui/imgui_demo.cpp
        ../imgui/backends/imgui_impl_opengl3.cpp
        ../imgui/backends/imgui_impl_sdl.cpp

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
        
        
        Renderer/GL/LineRenderer.cpp
        Renderer/GL/LineRenderer.hpp
        Renderer/RenderVertex.hpp
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
        UI/UiDictionary.hpp
        UI/UiBuilder.cpp
        Memory/Dictionary.hpp
        
        Sound/SoundEffect.cpp
        Memory/ResourcePool.hpp
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

        "Memory/CircularQueue.hpp"
        "Net/ReplicationManager.hpp"
        "Net/ReplicationManager.cpp"
        "Components/NetComponent.hpp"
        "Components/NetComponent.cpp"
        "Net/PlayerCommandRunner.hpp"
        "Net/PlayerCommandRunner.cpp"
        "Physics/PathFinding.hpp"
        "Physics/PathFinding.cpp"
        "Scene/EntityManager.cpp"
        Scene/EntitySerializer.hpp
		Scene/EntitySerializer.cpp

     	"Net/ServerGame.cpp"
		"ML/ML.hpp"
		"ML/ML.cpp"
		Components/PathFollowerComponent.cpp
		Components/PathFollowerComponent.hpp
		Components/LightComponent.hpp
		Net/FileTransfer.hpp
		Net/FileTransfer.cpp
        Net/SyncVar.cpp
		Resource/ResourceManager.hpp Resource/SpriteResource.hpp Resource/SpriteResource.cpp Resource/ResourceManager.cpp Resource/TilemapResource.hpp Resource/TilemapResource.cpp Resource/SpriteFontResource.hpp Resource/SpriteFontResource.cpp
		Resource/ShaderResource.cpp Resource/ShaderResource.hpp Components/ParticleSystemComponent.hpp Components/ParticleSystemComponent.cpp Scene/Isometric.hpp Scene/Isometric.cpp Components/IsometricSpriteComponent.hpp Components/IsometricSpriteComponent.cpp Resource/FileResource.hpp Resource/FileResource.cpp ML/UtilityAI.hpp   ML/GridSensor.hpp Renderer/SpriteEffect.hpp Renderer/SpriteEffect.cpp Renderer/Stage/RenderPipeline.hpp Renderer/Stage/RenderPipeline.cpp Resource/SpriteAtlasResource.hpp Resource/SpriteAtlasResource.cpp Resource/ResourceSettings.hpp Components/AnimatorComponent.hpp Components/AnimatorComponent.cpp ML/NeuralNetworkService.hpp)

message("SDL DIRS: ${SDL2_INCLUDE_DIRS}")

# Workaround because OALWrapper can't find SDL path
find_path(SDL_INCLUDE_DIR SDL.h PATH_SUFFIXES SDL2)

target_include_directories(Strife.Engine
        PUBLIC
            $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
            $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>/../
            $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../OALWrapper/include>
            $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../imgui>
        PRIVATE
            ${SDL2IMAGE_INCLUDE_DIRS}
            ../imgui/examples
            Renderer)

find_package(Vorbis REQUIRED)
find_package(Torch REQUIRED)
find_package(robin_hood REQUIRED)

target_link_libraries(Strife.Engine PUBLIC
    SDL2::SDL2
    SDL2::SDL2main
    SDL2::SDL2_image
    box2d::box2d
    OALWrapper
    SLikeNet
    Strife.ML
	robin_hood::robin_hood
    )

if(APPLE)
	target_link_libraries(Strife.Engine PUBLIC Vorbis::vorbis Vorbis::vorbisenc Vorbis::vorbisfile)
endif()

target_compile_definitions(Strife.Engine PUBLIC IMGUI_IMPL_OPENGL_LOADER_GL3W)

find_package(Microsoft.GSL CONFIG REQUIRED)
target_link_libraries(Strife.Engine PRIVATE Microsoft.GSL::GSL)
target_link_libraries(Strife.Engine PUBLIC tmxlite Strife.Common)

set_property(TARGET Strife.Engine PROPERTY CXX_STANDARD 17)

