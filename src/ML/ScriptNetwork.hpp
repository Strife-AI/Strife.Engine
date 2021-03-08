#pragma once

#include <torch/torch.h>
#include "ML.hpp"
#include "Scripting/Scripting.hpp"
#include "Strife.ML/TorchApiInternal.hpp"
#include "Resource/ScriptResource.hpp"

struct DynamicNetworkInput
{

};

struct DynamicNetworkOutput
{

};

struct ScriptNetwork : StrifeML::NeuralNetwork<DynamicNetworkInput, DynamicNetworkOutput, 1>
{
    void MakeDecision(Grid<const InputType> input, OutputType& output) override
    {

    }

    void TrainBatch(Grid<const SampleType> input, StrifeML::TrainingBatchResult& outResult) override
    {
        printf("Train batch\n");
        train(nullptr);
    }

    void BindCallbacks(std::shared_ptr<Script> script)
    {
        Log("Bind callbacks\n");
        if (!script->TryBindFunction(train))
        {
            Log("Failed to bind train function\n");
        }
    }

    ScriptFunction<void(TorchNetwork*)> train { "Train" };
};

struct ScriptTrainer : StrifeML::Trainer<ScriptNetwork>
{
    ScriptTrainer(ScriptResource* scriptResource)
        : StrifeML::Trainer<ScriptNetwork>(1, 1),
          scriptResource(scriptResource)
    {
        minSamplesBeforeStartingTraining = -1;
        Recompile();
    }

    void RunBatch() override
    {
        script->TryRecompileIfNewer();
        Trainer<ScriptNetwork>::RunBatch();
    }

    bool TryCreateBatch(Grid<SampleType> outBatch) override
    {
        return true;
    }

    void Recompile()
    {
        script = scriptResource->CreateScript();
        script->TryCompile();
    }

    void OnCreateNewNetwork(std::shared_ptr<NetworkType> newNetwork)
    {
        newNetwork->BindCallbacks(script);
    }

    ScriptResource* scriptResource;
    std::shared_ptr<Script> script;
};

struct ScriptDecider : StrifeML::Decider<ScriptNetwork>
{

};