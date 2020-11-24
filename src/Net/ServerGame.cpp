#include "ServerGame.hpp"

#include <thread>


#include "Engine.hpp"
#include "Renderer.hpp"
#include "SdlManager.hpp"
#include "Scene/Scene.hpp"
#include "System/Input.hpp"
#include "Tools/Console.hpp"
#include "Tools/MetricsManager.hpp"
#include "Tools/PlotManager.hpp"

ConsoleVar<bool> g_stressTest("stress-test", false, false);
int stressCount = 0;

static TimePointType lastFrameStart;    // FIXME: shouldn't be global

ConsoleVar<float> g_timeScale("time-scale", 1);

void SleepMicroseconds(unsigned int microseconds)
{
    std::this_thread::sleep_for(std::chrono::microseconds(microseconds));
}

void ThrottleFrameRate()
{
    auto currentFrameStart = std::chrono::high_resolution_clock::now();
    auto deltaTimeMicroseconds = std::chrono::duration_cast<std::chrono::microseconds>(currentFrameStart - lastFrameStart);
    auto realDeltaTime = static_cast<float>(deltaTimeMicroseconds.count()) / 1000000;
    auto desiredDeltaTime = 1.0f / Engine::GetInstance()->targetFps.Value();

    auto diffBias = 0.01f;
    auto diff = desiredDeltaTime - realDeltaTime - diffBias;
    auto desiredEndTime = lastFrameStart + std::chrono::microseconds((int)(desiredDeltaTime * 1000000));

    if (diff > 0)
    {
        SleepMicroseconds(1000000 * diff);
    }

    while (std::chrono::high_resolution_clock::now() < desiredEndTime)
    {

    }

    lastFrameStart = std::chrono::high_resolution_clock::now();
}

void BaseGameInstance::RunFrame()
{
    if (!g_stressTest.Value())
    {
        ThrottleFrameRate();
    }

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
        scene->lastFrameStart = currentFrameStart;
        scene->isFirstFrame = false;
    }

    auto deltaTimeMicroseconds = std::chrono::duration_cast<std::chrono::microseconds>(currentFrameStart - scene->lastFrameStart);
    auto realDeltaTime = static_cast<float>(deltaTimeMicroseconds.count()) / 1000000 * g_timeScale.Value();

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
            float fps = 1.0f / realDeltaTime;
            engine->GetMetricsManager()->GetOrCreateMetric("fps")->Add(fps);
        }

        input->GetMouse()->SetMouseScale(Vector2::Unit() * scene->GetCamera()->Zoom());
    }

    scene->lastFrameStart = currentFrameStart;

    if (sdlManager->ReceivedQuit())
    {
        // FIXME MW
        //_activeGame = false;
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
    renderer->BeginRender(camera, Vector2(0, 0), renderDeltaTime, scene->relativeTime);
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
        renderer->BeginRender(&uiCamera, uiCamera.TopLeft(), renderDeltaTime, scene->relativeTime);
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
    networkInterface.SendReliable(packet->systemAddress, response);
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
            Log("Assigned client id %d\n", clientId);
            serverAddress = packet->systemAddress;
            GetScene()->SendEvent(JoinedServerEvent(clientId));
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
