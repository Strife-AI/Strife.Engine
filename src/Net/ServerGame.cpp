#include "ServerGame.hpp"

#include <thread>


#include "Engine.hpp"
#include "Renderer.hpp"
#include "ReplicationManager.hpp"
#include "SdlManager.hpp"
#include "Scene/Scene.hpp"
#include "System/Input.hpp"
#include "Tools/Console.hpp"
#include "Tools/MetricsManager.hpp"
#include "Tools/PlotManager.hpp"

ConsoleVar<bool> g_stressTest("stress-test", false, false);
int stressCount = 0;

ConsoleVar<float> g_timeScale("time-scale", 1);

void BaseGameInstance::RunFrame(float currentTime)
{
    auto input = engine->GetInput();
    auto sdlManager = engine->GetSdlManager();

    if (!isHeadless)
    {
        input->Update();
        sdlManager->Update();
    }

    sceneManager.DoSceneTransition();

    Scene* scene = sceneManager.GetScene();

    auto currentFrameStart = std::chrono::high_resolution_clock::now();

    if (scene->isFirstFrame)
    {
        scene->lastFrameStart = currentTime;
        scene->isFirstFrame = false;
    }

    auto realDeltaTime = currentTime - scene->lastFrameStart;

    float renderDeltaTime = !engine->IsPaused()
        ? realDeltaTime
        : 0;

    scene->deltaTime = renderDeltaTime;

    if (!isHeadless)
    {
        engine->GetSoundManager()->UpdateActiveSoundEmitters(scene->deltaTime);
    }

    Entity* soundListener;
    if (scene->soundListener.TryGetValue(soundListener))
    {
        //_soundManager->SetListenerPosition(soundListener->Center(), soundListener->GetVelocity());
    }

    scene->UpdateEntities(renderDeltaTime);
    UpdateNetwork();

    bool allowConsole = !isHeadless && g_developerMode.Value();
    auto console = engine->GetConsole();

    if (allowConsole)
    {
        if (console->IsOpen())
        {
            console->HandleInput(input);
        }

        bool tildePressed = InputButton(SDL_SCANCODE_GRAVE).IsPressed();

        if (tildePressed)
        {
            if (console->IsOpen())
            {
                console->Close();
                engine->ResumeGame();
            }
            else
            {
                console->Open();
                engine->PauseGame();
            }
        }
    }

    if (!isHeadless)
    {
        Render(scene, realDeltaTime, renderDeltaTime);

        {
            static int count = 0;

            float fps = 1.0f / realDeltaTime;
            engine->GetMetricsManager()->GetOrCreateMetric("fps")->Add(fps);

            count = (count + 1) % 60;
            if(count == 0)
            {
                SDL_SetWindowTitle(engine->GetSdlManager()->Window(), std::to_string(fps).c_str());
            }
        }

        input->GetMouse()->SetMouseScale(Vector2::Unit() * scene->GetCamera()->Zoom());
    }

    scene->lastFrameStart = currentTime;

    if (!isHeadless)
    {
        if (sdlManager->ReceivedQuit())
        {
            engine->QuitGame();
        }
    }
}

void BaseGameInstance::Render(Scene* scene, float deltaTime, float renderDeltaTime)
{
    auto input = engine->GetInput();
    auto sdlManager = engine->GetSdlManager();
    auto renderer = engine->GetRenderer();
    auto console = engine->GetConsole();
    auto plotManager = engine->GetPlotManager();

    sdlManager->BeginRender();

    auto screenSize = sdlManager->WindowSize().AsVectorOfType<float>();
    scene->GetCamera()->SetScreenSize(screenSize);
    scene->GetCamera()->SetZoom(1);// screenSize.y / (1080 / 2));

    auto camera = scene->GetCamera();
    renderer->BeginRender(scene, camera, Vector2(0, 0), renderDeltaTime, scene->relativeTime);
    scene->RenderEntities(renderer);

    scene->SendEvent(RenderImguiEvent());

    // FIXME: add proper ambient lighting
    renderer->RenderRectangle(renderer->GetCamera()->Bounds(), Color(18, 21, 26), 0.99);
    //_renderer->RenderRectangle(_renderer->GetCamera()->Bounds(), Color(255, 255, 255), 0.99);
    renderer->DoRendering();

    // Render HUD
    renderer->SetRenderOffset(scene->GetCamera()->TopLeft());
    scene->RenderHud(renderer);

    if (g_stressTest.Value())
    {
        char text[1024];
        sprintf(text, "Time elapsed: %f hours", scene->relativeTime / 60.0f / 60.0f);

        renderer->RenderString(
            FontSettings(ResourceManager::GetResource<SpriteFont>("console-font"_sid)),
            text,
            Vector2(200, 200),
            -1);
    }

    renderer->RenderSpriteBatch();

    // Render UI
    {
        Camera uiCamera;
        uiCamera.SetScreenSize(scene->GetCamera()->ScreenSize());
        uiCamera.SetZoom(screenSize.y / 768.0f);
        renderer->BeginRender(scene, &uiCamera, uiCamera.TopLeft(), renderDeltaTime, scene->relativeTime);
        input->GetMouse()->SetMouseScale(Vector2::Unit() * uiCamera.Zoom());

        if (console->IsOpen())
        {
            console->Render(renderer, deltaTime);
        }

        console->Update();

        renderer->RenderSpriteBatch();
    }

    plotManager->RenderPlots();
    sdlManager->EndRender();
}

void ServerGame::HandleNewConnection(SLNet::Packet* packet)
{
    SLNet::BitStream response;
    int clientId = AddClient(packet->systemAddress);
    sceneManager.GetScene()->SendEvent(PlayerConnectedEvent(clientId));

    response.Write(PacketType::NewConnectionResponse);
    response.Write(clientId);
    response.Write(sceneManager.GetScene()->MapSegmentName().key);
    networkInterface.SendReliable(packet->systemAddress, response);

    Log("Client %d connected\n", clientId);
}

void ServerGame::UpdateNetwork()
{
    SLNet::Packet* packet;
    while (networkInterface.TryGetPacket(packet))
    {
        switch ((PacketType)packet->data[0])
        {
        case PacketType::NewConnection:
            HandleNewConnection(packet);
            break;

        case PacketType::UpdateRequest:
        {
            int clientId = GetClientId(packet->systemAddress);
            if (clientId == -1) continue;

            SLNet::BitStream request(packet->data, packet->length, false);
            SLNet::BitStream response;

            request.IgnoreBytes(1);
            onUpdateRequest(request, response, clientId);
            networkInterface.SendUnreliable(packet->systemAddress, response);
            break;
        }
        }
    }
}

void ClientGame::UpdateNetwork()
{
    SLNet::Packet* packet;
    while (networkInterface.TryGetPacket(packet))
    {
        switch ((PacketType)packet->data[0])
        {
        case PacketType::NewConnectionResponse:
        {
            SLNet::BitStream message(packet->data, packet->length, false);
            message.IgnoreBytes(1);

            message.Read(clientId);

            StringId mapName;
            message.Read(mapName.key);
            if(!sceneManager.TrySwitchScene(mapName))
            {
                Log("Failed to switch scene\n");
                return;
            }

            Log("Assigned client id %d\n", clientId);
            serverAddress = packet->systemAddress;
            sceneManager.GetNewScene()->SendEvent(JoinedServerEvent(clientId));
            break;
        }
        case PacketType::UpdateResponse:
        {
            SLNet::BitStream stream(packet->data, packet->length, false);
            stream.IgnoreBytes(1);
            onUpdateResponse(stream);
            break;
        }
        }
    }
}

int ServerGame::AddClient(const SLNet::AddressOrGUID& address)
{
    for (int i = 0; i < MaxClients; ++i)
    {
        if (clients[i].status != ClientConnectionStatus::Connected)
        {
            clients[i].status = ClientConnectionStatus::Connected;
            clients[i].clientId = i;
            clients[i].address = address;

            return i;
        }
    }

    // TODO MW
    FatalError("Server full");
}

int ServerGame::GetClientId(const SLNet::AddressOrGUID& address)
{
    for (auto& client : clients)
    {
        if (client.status == ClientConnectionStatus::Connected && client.address == address)
        {
            return client.clientId;
        }
    }

    return -1;
}
