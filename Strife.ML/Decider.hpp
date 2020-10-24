#pragma once

#include "CharacterAction.hpp"
#include <functional>
#include <thread>
#include "Memory/ConcurrentQueue.hpp"
#include "AICommon.hpp"

struct NeuralNetwork;

struct DeciderWorkItem
{
	DeciderWorkItem(std::function<void()> execute_)
		: execute(execute_)
	{
	    
	}

	std::function<void()> execute;
};

struct DecisionRequest
{
	DecisionRequest()
	{
		for (int i = 0; i < SequenceLength; ++i)
		{
			lastExperience[i].SetData(lastExperienceData[i]);

			for (int v = 0; v < 2; ++v)
			{
				lastFeatureData[i][v] = lastExperience[i].velocity[v];
			}
		}

		memset(lastExperienceData, 0, sizeof(lastExperienceData));
		memset(lastFeatureData, 0, sizeof(lastFeatureData));
	}

	DecompressedExperience lastExperience[SequenceLength];
	PerceptionGridType lastExperienceData[SequenceLength][PerceptionGridRows * PerceptionGridCols];
	float lastFeatureData[SequenceLength][InputFeaturesCount];
};

struct DecisionResponse
{
	DecisionResponse()
	    : id(-1), timeRequested(-1), action(CharacterAction::Nothing)
	{
	    
	}

	DecisionResponse(int id_, float timeRequested_, CharacterAction action_)
	    : id(id_),
	    timeRequested(timeRequested_),
	    action(action_)
	{
	    
	}

	int id;
	float timeRequested;
	CharacterAction action;
};

struct DeciderClient
{
	DeciderClient();

	ModelBinding modelBinding;
	std::shared_ptr<NeuralNetwork> network;
	ConcurrentQueue<DecisionResponse> responseChannel;
	int reloadCount = 0;
};

class Decider
{
public:
	~Decider();

	void RequestDecision(int clientId, DecompressedExperience experience[], int decisionId, float timeRequested);
	void BindClient(int clientId, const ModelBinding& binding);
	void FinishPendingTasks();
	bool TryGetNextDecision(int clientId, DecisionResponse& outResponse);

	void StartRunning();
	void StopRunning();

private:
	static constexpr int MaxClients = 1;

	void Run();
	void TryReloadModel(int clientId);

	ConcurrentQueue<DeciderWorkItem> _workQueue;
	DeciderClient _clients[MaxClients];
	std::unique_ptr<std::thread> _workerThread;
	bool _isDone = false;
};