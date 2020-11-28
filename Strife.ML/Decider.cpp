#include <torch/torch.h>

#include "Decider.hpp"
#include "ExperienceManager.hpp"
#include "NeuralNetwork.hpp"

DecompressedExperience* deciderExperience = nullptr;

DeciderClient::DeciderClient()
    : network(std::make_shared<OldNeuralNetwork>())
{
    network->eval();
}

Decider::~Decider()
{
    StopRunning();
}

void Decider::RequestDecision(int clientId, DecompressedExperience experience[], int decisionId, float timeRequested)
{
    auto request = std::make_shared<DecisionRequest>();

    if (experience != nullptr)
    {
        for (int i = 0; i < SequenceLength; ++i)
        {
            experience[i].CopyTo(request->lastExperience[i]);
        }
    }
    else
    {
        memset(request->lastExperienceData, 0, sizeof(request->lastExperienceData));
        memset(request->lastFeatureData, 0, sizeof(request->lastFeatureData));
    }

    _workQueue.Emplace([=]
    {
        auto& client = _clients[clientId];

        // Prevent making random decisions until we're sent at least one network
        if(client.reloadCount == 0)
        {
            client.responseChannel.Emplace(decisionId, timeRequested, CharacterAction::Nothing);
        }
        else
        {
            torch::Tensor spatialInput = torch::from_blob(request->lastExperienceData, { SequenceLength, 1, PerceptionGridRows, PerceptionGridCols }, torch::kLong);
            torch::Tensor featureInput = torch::from_blob(request->lastFeatureData, { SequenceLength, 1, InputFeaturesCount }, torch::kFloat);
            torch::Tensor output = client.network->forward(spatialInput, featureInput).squeeze();
            torch::Tensor index = std::get<1>(torch::max(output, 0));
            int maxNumber = *index.data_ptr<int64_t>();
            auto lastAction = static_cast<CharacterAction>(maxNumber);
            client.responseChannel.Emplace(decisionId, timeRequested, lastAction);
        }
    });
}

void Decider::BindClient(int clientId, const ModelBinding& binding)
{
    auto& client = _clients[clientId];
    client.modelBinding = binding;
}

void Decider::TryReloadModel(int clientId)
{
    auto& client = _clients[clientId];
    auto& channel = client.modelBinding.communicationChannel;
    Model newestModel;

    if (channel->IsEmpty())
    {
        return;
    }

    while (!channel->IsEmpty())
    {
        newestModel = channel->Front();
        channel->Dequeue();
        ++client.reloadCount;
    }

    torch::load(client.network, *newestModel.stream);
    auto network = _clients[clientId].network;

    torch::Device device(torch::kCPU);
    network->to(device);

    // Eval turns off layers that use is_training(), i.e. DropOut
    network->eval();

    // Remove all gradients since we are only doing inference.  In theory,
    // this should save on memory and performance slightly.
    for (auto param : network->parameters())
    {
        param.requires_grad_(false);
    }
}

void Decider::FinishPendingTasks()
{
    volatile int queueSize;

    do
    {
        queueSize = _workQueue.Size();
    } while (queueSize != 0);
}

bool Decider::TryGetNextDecision(int clientId, DecisionResponse& outResponse)
{
    auto& channel = _clients[clientId].responseChannel;
    if (!channel.IsEmpty())
    {
        outResponse = channel.Front();
        channel.Dequeue();
        return true;
    }
    else
    {
        return false;
    }
}

void Decider::StartRunning()
{
    _isDone = false;
    _workerThread = std::make_unique<std::thread>(&Decider::Run, this);
	std::cout << "Decider Thread Id: " << _workerThread->get_id() << std::endl;
}

void Decider::StopRunning()
{
    if (_workerThread == nullptr)
    {
        return;
    }

    _isDone = true;

    if (_workerThread->joinable())
    {
        _workerThread->join();
    }

    _workerThread = nullptr;
}

void Decider::Run()
{
    while (!_isDone)
    {
        if (_workQueue.Size() > 0)
        {
            auto item = _workQueue.Front();
            item.execute();
            _workQueue.Dequeue();
        }

        for(int i = 0; i < MaxClients; ++i)
        {
            TryReloadModel(i);
        }
    }
}