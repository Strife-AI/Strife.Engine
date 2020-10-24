#pragma once

#include "LearningRateSchedule.hpp"
#include "CharacterAction.hpp"
#include <functional>
#include "Memory/ConcurrentQueue.hpp"
#include "ExperienceManager.hpp"
#include <thread>
#include "Memory/EnumDictionary.hpp"
#include <random>
#include "AICommon.hpp"

struct DataBatch;
struct NeuralNetwork;
struct Metric;

struct TrainerWorkItem
{
	TrainerWorkItem(const std::string& description_, std::function<void()> execute_)
		: description(description_),
        execute(execute_)
	{

	}

	std::string description;
	std::function<void()> execute;
};

namespace c10
{
	class Device;
}

template<typename T, int TotalItems>
struct ConfusionMatrix
{
	ConfusionMatrix()
	{
		Clear();
	}

	void Clear()
	{
		for (auto& row : matrix)
		{
			for (auto& column : row)
			{
				column = 0;
			}
		}
	}

	EnumDictionary<T, EnumDictionary<T, int, TotalItems>, TotalItems> matrix;
};

struct TrainerClient
{
	static constexpr int MaxMapSegments = 1;

	TrainerClient();

	SampleManager& GetSampleManager(int segmentId)
	{
	    if(segmentId < 0 || segmentId >= MaxMapSegments)
	    {
			FatalError("Sample manager for segment %d is out of range", segmentId);
	    }

		return sampleManagersBySegmentId[0];
	}

	SampleManager& GetCurrentSegmentSampleManager()
	{
		return GetSampleManager(0);
	}

	void MoveToNextSegment()
	{
		AllocateSampleManager();
	}

	void AddSampleToCurrentSegment(int experienceId, CharacterAction action)
	{
	    sampleManagersBySegmentId[0].AddSample(experienceId, action);
	}

	SampleManager& AllocateSampleManager()
	{
		auto& sampleManager = sampleManagersBySegmentId[0];
		totalSegments = 1;
		sampleManager.Reset();
		return sampleManager;
	}

	gsl::span<SampleManager> GetSampleManagers()
	{
		return gsl::span<SampleManager>(sampleManagersBySegmentId, 1);
	}

	void Reset()
	{
		totalSegments = 1;
		totalBatchesRun = 0;
		runBatchesContinuously = true;

		for(auto& sampleManager : sampleManagersBySegmentId)
		{
			sampleManager.Reset();
		}

		confusionMatrix.Clear();
	}

	void SetNetwork(std::shared_ptr<NeuralNetwork> network);

	ModelBinding modelBinding;
	SampleManager sampleManagersBySegmentId[MaxMapSegments];

	std::shared_ptr<NeuralNetwork> pendingNetwork;
	Model lastNetwork;
	std::unique_ptr<c10::Device> devicePtr;
	
	int totalSegments = 0;
	int firstNewSegment = 0;

	ConfusionMatrix<CharacterAction, (int)CharacterAction::TotalActions> confusionMatrix;
	int totalCorrectActions = 0;
	int totalActions = 0;

	bool trainingEnabled = false;

	bool runBatchesContinuously = true;
	int batchesRemaining = 0;
	int totalBatchesRun = 0;
};

struct RunBatchResult
{
    float loss;
    float accuracy;
};

class Trainer
{
public:
	Trainer(ILearningRateSchedule* lrSchedule, Metric* lrMetric, Metric* lossMetric, Metric* trainPerSecond, Metric* accuracyMetric, Metric* totalAccuracyMetric);
	~Trainer();

	void FinishPendingTasks();
	void DiscardPendingTasks();
	void StartRunning();
	void StopRunning();

	void BindClient(int clientId, const ModelBinding& binding);
	void AddExperience(int clientId, const Sample& sample);
	void DiscardPendingModel(int clientId);
	void SaveWeightsToFile(int clientId, const std::string& fileName);
	void BuildModelFromSegmentFiles(int clientId, const std::vector<std::string>& files);
	void AddSamplesFromFile(int clientId, const std::string& file);
	void SaveSegmentToFile(int clientId, int segmentId, const std::string& fileName);
	void MoveToNextSegment(int clientId);
	void WarmUp(int clientId);
	void GetLastBatch(gsl::span<DecompressedExperience, SequenceLength>& outSequence, int id);
	void EnableTraining(int clientId);
	void DisableTraining(int clientId);
	int TotalSamples(int clientId);
	bool TrainingEnabled(int clientId) const;
	void SetRemainingBatches(int clientId, int count);
	void RunContinuously(int clientId);

	int TotalSegmentSamples(int clientId, int segmentId);
	int FirstNewSegmentId(int clientId) const { return _clients[clientId].firstNewSegment; }
	int TotalBatchesRun(int clientId) const { return _clients[clientId].totalBatchesRun; }

	auto GetConfusionMatrix(int clientId) const { return _clients[clientId].confusionMatrix; }
	void ResetConfusionMatrix(int clientId);

private:
	static constexpr int MaxClients = 1;

	void Run();
	void AddWorkItem(const TrainerWorkItem& item);
	void Train(TrainerClient& client, gsl::span<SampleManager> sampleManagers, int totalBatches);
	void PromotePendingToActive(int clientId);
	void Train(int clientId, int segmentId);
	void TrainBatch(TrainerClient& client, const DataBatch& batch);
	RunBatchResult RunBatch(TrainerClient& client, const DataBatch& batch);
	void DoClientTraining(int clientId);

	ConcurrentQueue<TrainerWorkItem> _workQueue;
	TrainerClient _clients[MaxClients];
	std::unique_ptr<std::thread> _workerThread;
	bool _isDone = false;

	long long _nextItemSequence = 0;
	long long _processedItemSequence = 0;
	long long _discardUntil = -1;

	ILearningRateSchedule* _learningRateSchedule;
	Metric* _lrMetric;
	Metric* _trainPerSecond;
	Metric* _lossMetric;
	Metric* _accuracyMetric;
	Metric* _totalAccuracyMetric;
};