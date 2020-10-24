#include "Trainer.hpp"
#include "NeuralNetwork.hpp"
#include <random>

#include "System/BinaryStreamReader.hpp"
#include "System/BinaryStreamWriter.hpp"
#include "Tools/MetricsManager.hpp"
#include "Tools/RawFile.hpp"
#include "Tools/ConsoleVar.hpp"

static const int BatchSize = 16;
std::default_random_engine generator;

#ifndef RELEASE_BUILD
ConsoleVar<bool> g_saveWeights("save", false, false);
ConsoleVar<bool> g_enableTrainer("trainer", true, true);
ConsoleVar<bool> g_useCuda("cuda", false, true);
ConsoleVar<bool> g_transferLearning("transfer-learning", true, false);
ConsoleVar<std::string> g_weightsResourceName("weights", "nn-weights", true);
#else
ConsoleVar<bool> g_saveWeights("save", false, false);
ConsoleVar<bool> g_enableTrainer("trainer", true, false);
ConsoleVar<bool> g_useCuda("cuda", false, false);
ConsoleVar<bool> g_transferLearning("transfer-learning", true, false);
ConsoleVar<std::string> g_weightsResourceName("weights", "nn-weights", false);
#endif

struct DataBatch
{
    torch::Tensor SpatialInput;
    torch::Tensor FeatureInput;
    torch::Tensor Labels;

    long long experienceData[SequenceLength][BatchSize][PerceptionGridRows][PerceptionGridCols];
    float featureData[SequenceLength][BatchSize][InputFeaturesCount];
    long long labels[BatchSize];
    void UpdateTensors(const TrainerClient& client);
};

void DataBatch::UpdateTensors(const TrainerClient& client)
{
    SpatialInput = torch::from_blob(experienceData,
        { SequenceLength, BatchSize, PerceptionGridRows, PerceptionGridCols },
        torch::dtype(torch::kLong)).to(*client.devicePtr);
    FeatureInput = torch::from_blob(featureData,
        { SequenceLength, BatchSize, InputFeaturesCount },
        torch::dtype(torch::kFloat)).to(*client.devicePtr);
    Labels = torch::from_blob(labels,
        { BatchSize },
        torch::dtype(torch::kLong)).to(*client.devicePtr);
}

bool CreateBatch(const TrainerClient& client, gsl::span<SampleManager> sampleManagers, DataBatch& outBatch)
{
    std::uniform_int_distribution<int> actionTypeDistribution(0, (int)CharacterAction::TotalActions - 1);

    if(!sampleManagers[0].HasSamples())
    {
        return false;
    }

    for (int i = 0; i < BatchSize; ++i)
    {
        bool gotSample = false;
        DecompressedExperience nextSamples[SequenceLength];

        for (int sampleId = 0; sampleId < SequenceLength; ++sampleId)
        {
            auto& sample = nextSamples[sampleId];
            sample.perceptionGrid.Set(PerceptionGridRows, PerceptionGridCols, &outBatch.experienceData[sampleId][i][0][0]);
        }

        while (!gotSample)
        {
            CharacterAction action = static_cast<CharacterAction>(actionTypeDistribution(generator));

            gotSample = sampleManagers[0].TryGetRandomSample(action, gsl::span<DecompressedExperience>(nextSamples));

            outBatch.labels[i] = static_cast<PerceptionGridType>(action);

            for (int sampleId = 0; sampleId < SequenceLength; ++sampleId)
            {
                auto& sample = nextSamples[sampleId];

                for (int v = 0; v < 2; ++v)
                {
                    outBatch.featureData[sampleId][i][v] = sample.velocity[v];
                }
            }
        }
    }

    outBatch.UpdateTensors(client);

    return true;
}

RunBatchResult Trainer::RunBatch(TrainerClient& client, const DataBatch& batch)
{
    static float lastLearningRate = -1;

	_learningRateSchedule->Step();
	auto currentLr = _learningRateSchedule->GetCurrentLearningRate();
    if (lastLearningRate != currentLr) {
        client.pendingNetwork->optimizer = std::make_shared<torch::optim::Adam>(client.pendingNetwork->parameters(), currentLr);
        lastLearningRate = currentLr;
    }
	_lrMetric->Add(lastLearningRate);

    auto& optimizer = client.pendingNetwork->optimizer;
    optimizer->zero_grad();

    torch::Tensor prediction = client.pendingNetwork->forward(batch.SpatialInput, batch.FeatureInput).squeeze();
    torch::Tensor loss = torch::nn::functional::nll_loss(prediction, batch.Labels);

    loss.backward();
    optimizer->step();
 
	auto cpuDevice = torch::device(torch::kCPU);
    auto max = std::get<1>(prediction.max(1)).to(cpuDevice);
    auto correct = max.eq(batch.Labels.to(cpuDevice));
    auto totalCorrect = correct.sum().item<int>();

    for(int i = 0; i < correct.size(0); ++i)
    {
        int actual = batch.labels[i];
        int predicted = ((int*)max.data_ptr())[i];

        client.confusionMatrix.matrix[actual][predicted] += 1;
    }

    client.totalCorrectActions += totalCorrect;
    client.totalActions += BatchSize;

    RunBatchResult result = { loss.item<float>(), static_cast<float>(totalCorrect) / BatchSize };
    return result;
}

void Trainer::TrainBatch(TrainerClient& client, const DataBatch& batch)
{
    auto start = std::chrono::high_resolution_clock::now();

    RunBatchResult result = RunBatch(client, batch);
    _lossMetric->Add(result.loss);
    _accuracyMetric->Add(result.accuracy);
    _totalAccuracyMetric->Add(static_cast<float>(client.totalCorrectActions) / client.totalActions);

    auto end = std::chrono::high_resolution_clock::now();

    auto timeMicroseconds = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    auto tps = 1000000.0f / timeMicroseconds.count();
    _trainPerSecond->Add(tps);
}

TrainerClient::TrainerClient()
    : pendingNetwork(std::make_shared<NeuralNetwork>())
{
    const auto usingCuda = torch::cuda::is_available() && g_useCuda.Value();
    printf("%s\n", usingCuda ? "Training on CUDA GPU." : "Training on CPU.");

    auto deviceType = usingCuda ? torch::kCUDA : torch::kCPU;
    devicePtr = std::make_unique<torch::Device>(deviceType);
    SetNetwork(pendingNetwork);
}

void TrainerClient::SetNetwork(std::shared_ptr<NeuralNetwork> network)
{
    pendingNetwork = network;
	pendingNetwork->optimizer = std::make_shared<torch::optim::Adam>(pendingNetwork->parameters(), 1e-3); // todo brendan this shouldnt be locked at 1e-3
    pendingNetwork->train();
    pendingNetwork->to(*devicePtr);
}

Trainer::Trainer(ILearningRateSchedule* lrSchedule, Metric* lrMetric, Metric* lossMetric, Metric* trainPerSecond, Metric* accuracyMetric, Metric* totalAccuracyMetric)
    : _learningRateSchedule(lrSchedule),
	_lrMetric(lrMetric),
	_trainPerSecond(trainPerSecond),
    _lossMetric(lossMetric),
    _accuracyMetric(accuracyMetric),
    _totalAccuracyMetric(totalAccuracyMetric)
{
}

Trainer::~Trainer()
{
    StopRunning();
}

void Trainer::FinishPendingTasks()
{
    volatile int queueSize;

    do
    {
        queueSize = _workQueue.Size();
    } while (queueSize != 0);

    
}

void Trainer::DiscardPendingTasks()
{
    _discardUntil = _nextItemSequence;
}

void Trainer::StartRunning()
{
    std::cout << "Hello from trainer!" << std::endl;

    std::cout << std::boolalpha;
    std::cout << "CUDA is available: " << torch::cuda::is_available() << std::endl;

    std::cout << torch::show_config() << std::endl;
    std::cout << "Torch Inter-Op Threads: " << torch::get_num_interop_threads() << std::endl;
    std::cout << "Torch Intra-Op Threads: " << torch::get_num_threads() << std::endl;

    _isDone = false;
    _workerThread = std::make_unique<std::thread>(&Trainer::Run, this);
    std::cout << "Trainer Thread Id: " << _workerThread->get_id() << std::endl;
}

void Trainer::StopRunning()
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

void Trainer::BindClient(int clientId, const ModelBinding& binding)
{
    _clients[clientId].modelBinding = binding;
}

void Trainer::AddExperience(int clientId, const Sample& sample)
{
    Sample copy = sample;

    AddWorkItem(TrainerWorkItem("Add Experience", [=]()
    {
        int experienceId = _clients[clientId].GetCurrentSegmentSampleManager().experienceManager.CreateExperience(copy.compressedRectangles, copy.velocity);

        if (experienceId != -1)
        {
            _clients[clientId].AddSampleToCurrentSegment(experienceId, copy.action);
        }
    }));
}

void Trainer::PromotePendingToActive(int clientId)
{
    auto& client = _clients[clientId];

    auto stream = std::make_shared<std::stringstream>();
    torch::save(client.pendingNetwork, *stream);
    client.modelBinding.communicationChannel->Enqueue(Model(stream));
    client.lastNetwork.stream = stream;
}

void Trainer::DiscardPendingModel(int clientId)
{
    AddWorkItem(TrainerWorkItem("Discard pending model", [=]()
    {
        auto& client = _clients[clientId];
        torch::load(client.pendingNetwork, *client.lastNetwork.stream);
        client.GetCurrentSegmentSampleManager().Reset();
        printf("Discarded pending model\n");
    }));
}

void Trainer::SaveWeightsToFile(int clientId, const std::string& baseFileName)
{
    AddWorkItem(TrainerWorkItem("Save Weights To File", [=]()
    {
        auto& client = _clients[clientId];
        if (client.devicePtr->is_cuda())
        {
            const auto deviceType = torch::kCPU;
            const torch::Device device(deviceType);
            client.pendingNetwork->to(device);
        }

        torch::save(client.pendingNetwork, baseFileName);
        printf("Saved trainer weights to file %s\n", baseFileName.c_str());

        if (client.devicePtr->is_cuda())
        {
            client.pendingNetwork->to(*client.devicePtr);
        }
    }));
}

static DataBatch g_batch;     // Allocated statically to prevent a stack overflow

void Trainer::Train(TrainerClient& client, gsl::span<SampleManager> sampleManagers, int totalBatches)
{
    bool converged = false;
    int count = 0;

    while (!converged)
    {
        if(!CreateBatch(client, sampleManagers, g_batch))
        {
            return;
        }

        TrainBatch(client, g_batch);
        ++client.totalBatchesRun;
        converged = ++count >= totalBatches;
    }

    if (g_saveWeights.Value())
    {
        SaveWeightsToFile(0, "nn-weights.pt");
        g_saveWeights.SetValue(false);
    }
}

void Trainer::BuildModelFromSegmentFiles(int clientId, const std::vector<std::string>& files)
{
    AddWorkItem(TrainerWorkItem("Build model from segment files", [=]()
    {
        int totalFiles = files.size();
        auto& client = _clients[clientId];

        client.Reset();

        for (int i = 0; i < totalFiles; ++i)
        {
            auto& sampleManager = client.AllocateSampleManager();
            BinaryStreamReader reader;
            reader.Open(files[i].c_str());
            sampleManager.Deserialize(reader);
            reader.Close();

            printf("Trainer loaded %s with %d samples\n", files[i].c_str(), sampleManager.experienceManager.TotalExperiences());
        }

        client.AllocateSampleManager();
        client.firstNewSegment = client.totalSegments - 1;

        const auto useTransferLearning = g_transferLearning.Value();

        Log("Using transfer-learning: %s\n", useTransferLearning ? "true" : "false");
        if (useTransferLearning)
        {
            auto model = std::make_shared<NeuralNetwork>();
            auto& weights = ResourceManager::GetResource<RawFile>(StringId(g_weightsResourceName.Value()))->data;

            std::string str(reinterpret_cast<char*>(weights.data()), reinterpret_cast<char*>(weights.data() + weights.size()));
            std::stringstream is(str);
            torch::load(model, is);

            for (auto parameter : model->parameters())
            {
                parameter.requires_grad_(false);
            }

            torch::nn::init::uniform_(model->dense->weight);
            for (auto parameter : model->dense->parameters())
            {
                parameter.requires_grad_(true);
            }

            client.SetNetwork(model);
        }
        // else we don't need to do anything since the network was set in the constructor

        if (!files.empty())
        {
            Train(client, client.GetSampleManagers(), files.size() * 25);
        }
    }));
}

void Trainer::AddSamplesFromFile(int clientId, const std::string& file)
{
    AddWorkItem(TrainerWorkItem("Load segment file", [=]
    {
        BinaryStreamReader reader;
        reader.Open(file.c_str());
        auto& sampleManager = _clients[clientId].GetCurrentSegmentSampleManager();
        sampleManager.Deserialize(reader);
        reader.Close();

        printf("Trainer loaded %s with %d samples\n", file.c_str(), sampleManager.experienceManager.TotalExperiences());
    }));
}

void Trainer::EnableTraining(int clientId)
{
    _clients[clientId].trainingEnabled = true;
}

void Trainer::DisableTraining(int clientId)
{
    _clients[clientId].trainingEnabled = false;
}

int Trainer::TotalSamples(int clientId)
{
    return _clients[clientId].GetCurrentSegmentSampleManager().experienceManager.TotalExperiences();
}

bool Trainer::TrainingEnabled(int clientId) const
{
    return _clients[clientId].trainingEnabled;
}

void Trainer::SetRemainingBatches(int clientId, int count)
{
    _clients[clientId].batchesRemaining = count;
    _clients[clientId].runBatchesContinuously = false;
}

void Trainer::RunContinuously(int clientId)
{
    _clients[clientId].runBatchesContinuously = true;
    _clients[clientId].batchesRemaining = 0;
}

void Trainer::Train(int clientId, int segmentId)
{
    auto& client = _clients[clientId];
    Train(client, client.GetSampleManagers(), 1);
    PromotePendingToActive(clientId);
}

void Trainer::MoveToNextSegment(int clientId)
{
    AddWorkItem(TrainerWorkItem("Move to next segment", [=]
    {
        _clients[clientId].MoveToNextSegment();
    }));
}

void Trainer::WarmUp(int clientId)
{
    AddWorkItem(TrainerWorkItem("Trainer warm up", [=]
    {
        memset(g_batch.labels, 0, sizeof(g_batch.labels));
        memset(g_batch.experienceData, 0, sizeof(g_batch.experienceData));
        memset(g_batch.featureData, 0, sizeof(g_batch.featureData));

        auto& client = _clients[clientId];
        g_batch.UpdateTensors(client);
        RunBatch(client, g_batch);

        client.Reset();
    }));
}

void Trainer::GetLastBatch(gsl::span<DecompressedExperience, SequenceLength>& outSequence, int id)
{
    for (int i = 0; i < SequenceLength; ++i)
    {
        outSequence[i].perceptionGrid.Set(PerceptionGridRows, PerceptionGridCols, &g_batch.experienceData[i][id][0][0]);
    }
}

void Trainer::SaveSegmentToFile(int clientId, int segmentId, const std::string& fileName)
{
    AddWorkItem(TrainerWorkItem("Save segment " + std::to_string(segmentId), [=]()
    {
        BinaryStreamWriter writer;
        _clients[clientId].GetSampleManager(segmentId).Serialize(writer);
        writer.WriteToFile(fileName.c_str());
    }));
}

int Trainer::TotalSegmentSamples(int clientId, int segmentId)
{
    return _clients[clientId]
        .GetSampleManager(segmentId)
        .experienceManager
        .TotalExperiences();
}

void Trainer::DoClientTraining(int clientId)
{
    auto& client = _clients[clientId];

    if (client.trainingEnabled)
    {
        bool runBatch;

        if(client.runBatchesContinuously)
        {
            runBatch = true;
        }
        else if(client.batchesRemaining > 0)
        {
            runBatch = true;
            --client.batchesRemaining;
        }
        else
        {
            runBatch = false;
            client.trainingEnabled = false;
        }

        if (runBatch)
        {
            Train(clientId, 0);
        }
    }
}

void Trainer::ResetConfusionMatrix(int clientId)
{
    _clients[clientId].confusionMatrix.Clear();
}

void Trainer::Run()
{
    while (!_isDone)
    {
        while (_workQueue.Size() > 0)
        {
            auto item = _workQueue.Front();

            _processedItemSequence++;
            bool discardItem = _processedItemSequence < _discardUntil;

            if (!discardItem && g_enableTrainer.Value())
            {
                item.execute();
            }

            _workQueue.Dequeue();
        }

        for (int i = 0; i < MaxClients; ++i)
        {
            DoClientTraining(i);
        }
    }
}

void Trainer::AddWorkItem(const TrainerWorkItem& item)
{
    _workQueue.Enqueue(item);
    ++_nextItemSequence;
}
