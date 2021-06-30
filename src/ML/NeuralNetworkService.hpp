#pragma once

#include "Memory/CircularQueue.hpp"
#include "Scene/Scene.hpp"
#include "Scene/IEntityEvent.hpp"
#include "ML/ML.hpp"

template<typename TInput>
struct InputCircularBufferAllocator
{
    using InputCircularBuffer = CircularQueue<TInput>;

    InputCircularBufferAllocator(int maxBuffers, int circularBufferSize)
        : bufferPool(std::make_unique<InputCircularBuffer[]>(maxBuffers)),
          inputs(std::make_unique<TInput[]>(maxBuffers * circularBufferSize)),
          freeBuffers(bufferPool.get(), maxBuffers),
          circularBufferSize(circularBufferSize)
    {

    }

    InputCircularBuffer* Allocate();

    void Free(InputCircularBuffer* buffer)
    {
        freeBuffers.Return(buffer);
    }

    std::unique_ptr<InputCircularBuffer[]> bufferPool;
    std::unique_ptr<TInput[]> inputs;
    FreeList<InputCircularBuffer> freeBuffers;
    int circularBufferSize;
};

template<typename TInput>
CircularQueue<TInput>* InputCircularBufferAllocator<TInput>::Allocate()
{
    auto buffer = freeBuffers.Borrow();

    // Returning a node to the free list overwrites its data, so this has to be initialized every time its allocated
    int sampleId = buffer - bufferPool.get();
    TInput* storageStart = inputs.get() + sampleId * circularBufferSize;

    new (buffer) InputCircularBuffer(storageStart, circularBufferSize);

    return buffer;
}

template<typename TNetwork>
struct DecisionBatch
{
    using InputType = typename TNetwork::InputType;
    using OutputType = typename TNetwork::OutputType;
    using InputCircularBuffer = CircularQueue<InputType>;

    DecisionBatch(int maxBatchSize, int sequenceLength, StrifeML::NetworkContext<TNetwork>* networkContext)
        : sequenceLength(sequenceLength),
          decisionInput(maxBatchSize * sequenceLength),
          decisionOutput(maxBatchSize),
          networkContext(networkContext)
    {
        entitiesInBatch.reserve(maxBatchSize);
    }

    void ResetBatch();
    void AddToBatch(Entity* entity, InputCircularBuffer& buffer);

    bool HasBatchInProgress() const
    {
        return decisionInProgress != nullptr;
    }

    bool BatchIsComplete() const
    {
        return decisionInProgress->IsComplete();
    }

    void StartBatchIfAnyEntities();

    int sequenceLength;
    std::vector<EntityReference<Entity>> entitiesInBatch;
    std::shared_ptr<StrifeML::MakeDecisionWorkItem<TNetwork>> decisionInProgress;
    StrifeML::MlUtil::SharedArray<InputType> decisionInput;
    StrifeML::MlUtil::SharedArray<OutputType> decisionOutput;
    StrifeML::NetworkContext<TNetwork>* networkContext;
};

template<typename TNetwork>
void DecisionBatch<TNetwork>::ResetBatch()
{
    entitiesInBatch.clear();
    decisionInProgress = nullptr;
}

template<typename TNetwork>
void DecisionBatch<TNetwork>::AddToBatch(Entity* entity, DecisionBatch::InputCircularBuffer& buffer)
{
    int row = entitiesInBatch.size();
    entitiesInBatch.emplace_back(entity);

    int col = 0;
    for (InputType& sample : buffer)
    {
        decisionInput.data.get()[row * sequenceLength + col] = sample;
        ++col;
    }
}

template<typename TNetwork>
void DecisionBatch<TNetwork>::StartBatchIfAnyEntities()
{
    if (entitiesInBatch.size() > 0)
    {
        decisionInProgress = networkContext->decider->MakeDecision(
            decisionInput,
            decisionOutput,
            networkContext->sequenceLength,
            entitiesInBatch.size());
    }
}

template<typename TEntity, typename TNetwork>
struct NeuralNetworkService : ISceneService, IEntityObserver
{
    using InputType = typename TNetwork::InputType;
    using OutputType = typename TNetwork::OutputType;
	using SampleType = typename TNetwork::SampleType;
    using TrainerType = StrifeML::Trainer<TNetwork>;

    using InputCircularBuffer = CircularQueue<InputType>;

    NeuralNetworkService(StrifeML::NetworkContext<TNetwork>* networkContext, int maxEntitiesInBatch)
        : networkContext(networkContext),
          bufferAllocator(maxEntitiesInBatch, networkContext->sequenceLength + 1),
          decisionBatch(maxEntitiesInBatch, networkContext->sequenceLength, networkContext)
    {

    }

    void OnAdded() override;
    void ReceiveEvent(const IEntityEvent& ev) override;

protected:
    virtual void CollectInput(TEntity* entity, InputType& input) = 0;

    void ForEachEntity(const std::function<void(TEntity*)>& func);

private:
    void CollectInputs();
    void StartMakingDecision();
    void BroadcastDecisions();
    void OnEntityAdded(Entity* entity) override;
    void OnEntityRemoved(Entity* entity) override;

    virtual void ReceiveDecision(TEntity* entity, OutputType& output)
    {

    }

    virtual void CollectTrainingSamples(TrainerType* trainer)
    {

    }

    virtual bool IncludeEntityInBatch(TEntity* entity)
    {
        return true;
    }

    virtual bool TrackEntity(TEntity* entity)
    {
        return true;
    }

    float makeDecisionTimer = 0.0f;
    float makeDecisionFrequency = 1.0f;

    float collectInputTimer = 0.0f;
    float collectInputFrequency = 1.0f;

    float collectTrainingSampleTimer = 0.0f;
    float collectTrainingSampleFrequency = 1.0f;

    StrifeML::NetworkContext<TNetwork>* networkContext;

    robin_hood::unordered_flat_map<TEntity*, InputCircularBuffer*> samplesByEntity;

    InputCircularBufferAllocator<InputType> bufferAllocator;
    DecisionBatch<TNetwork> decisionBatch;
};

template<typename TEntity, typename TNetwork>
void NeuralNetworkService<TEntity, TNetwork>::OnAdded()
{
    scene->AddEntityObserver<TEntity>(this);
}

template<typename TEntity, typename TNetwork>
void NeuralNetworkService<TEntity, TNetwork>::ReceiveEvent(const IEntityEvent& ev)
{
    if (ev.Is<UpdateEvent>())
    {
        // Collect inputs
        {
            collectInputTimer -= scene->deltaTime;
            if (collectInputTimer <= 0)
            {
                CollectInputs();
                collectInputTimer = 1.0f / collectInputFrequency;
            }
        }

        // Make decisions
        {
            makeDecisionTimer -= scene->deltaTime;

            if (decisionBatch.HasBatchInProgress())
            {
                if (decisionBatch.BatchIsComplete())
                {
                    BroadcastDecisions();
                    decisionBatch.ResetBatch();
                }
            }
            else
            {
                if (makeDecisionTimer <= 0)
                {
                    StartMakingDecision();
                    makeDecisionTimer = 1.0f / makeDecisionFrequency;
                }
            }
        }

        // Collect training samples
        {
            collectTrainingSampleTimer -= scene->deltaTime;
            if (collectTrainingSampleTimer <= 0.0f)
            {
                CollectTrainingSamples(networkContext->trainer);
                collectTrainingSampleTimer = 1.0f / collectTrainingSampleFrequency;
            }
        }
    }
}

template<typename TEntity, typename TNetwork>
void NeuralNetworkService<TEntity, TNetwork>::ForEachEntity(const std::function<void(TEntity*)>& func)
{
    for (auto& entityBufferPair : samplesByEntity)
    {
        func(entityBufferPair.first);
    }
}

template<typename TEntity, typename TNetwork>
void NeuralNetworkService<TEntity, TNetwork>::CollectInputs()
{
    for (auto& entityBufferPair : samplesByEntity)
    {
        TEntity* entity = entityBufferPair.first;

        if (!IncludeEntityInBatch(entity))
        {
            continue;
        }

        InputCircularBuffer* buffer = entityBufferPair.second;
        InputType* input = buffer->DequeueHeadIfFullAndAllocate();
        CollectInput(entityBufferPair.first, *input);
    }
}

template<typename TEntity, typename TNetwork>
void NeuralNetworkService<TEntity, TNetwork>::StartMakingDecision()
{
    for (auto entityBufferPair : samplesByEntity)
    {
        TEntity* entity = entityBufferPair.first;

        if (!IncludeEntityInBatch(entity))
        {
            continue;
        }

        InputCircularBuffer* buffer = entityBufferPair.second;

        // Include in batch if there are enough inputs in the sequence
        bool includeInBatch = buffer->IsFull();
        if (includeInBatch)
        {
            decisionBatch.AddToBatch(entity, *buffer);
        }
    }

    decisionBatch.StartBatchIfAnyEntities();
}

template<typename TEntity, typename TNetwork>
void NeuralNetworkService<TEntity, TNetwork>::BroadcastDecisions()
{
    for (int i = 0; i < decisionBatch.entitiesInBatch.size(); ++i)
    {
        Entity* entity;

        // Make sure the entity wasn't destroyed in the middle of making a decision
        if (decisionBatch.entitiesInBatch[i].TryGetValue(entity))
        {
            TEntity* entityAsTEntity = static_cast<TEntity*>(entity);
            ReceiveDecision(entityAsTEntity, decisionBatch.decisionOutput.data.get()[i]);
        }
    }
}

template<typename TEntity, typename TNetwork>
void NeuralNetworkService<TEntity, TNetwork>::OnEntityAdded(Entity* entity)
{
    // Safe to do static_cast<> since we're only subscribing to entities of one type
    TEntity* entityAsTEntity = static_cast<TEntity*>(entity);
    if (TrackEntity(entityAsTEntity))
    {
        auto buffer = bufferAllocator.Allocate();
        samplesByEntity[entityAsTEntity] = buffer;
    }
}

template<typename TEntity, typename TNetwork>
void NeuralNetworkService<TEntity, TNetwork>::OnEntityRemoved(Entity* entity)
{
    TEntity* entityAsTEntity = static_cast<TEntity*>(entity);
    auto it = samplesByEntity.find(entityAsTEntity);
    if (it != samplesByEntity.end())
    {
        bufferAllocator.Free(it->second);
    }

    samplesByEntity.erase(it);
}
