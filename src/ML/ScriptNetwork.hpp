#pragma once

#include <torch/torch.h>
#include <Strife.ML/TorchApi.h>
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
        try
        {
            DoScriptCall([=] { train(); });
        }
        catch (...)
        {
            // TODO
        }
    }

    void BindCallbacks(std::shared_ptr<Script> script, bool runSetup)
    {
        if (!script->TryBindFunction(train))
        {
            Log("Failed to bind train function\n");
        }

        if (script->TryBindFunction(setup) && runSetup)
        {
            DoScriptCall([=] { setup(); });
        }
    }

    template<typename TFunc>
    void DoScriptCall(TFunc func)
    {
        auto scriptState = Scripting::GetScriptingState();
        scriptState->network = &networkState;
        func();
        scriptState->tensors.objects.clear();
        scriptState->network = nullptr;
    }

    ScriptFunction<void()> setup { "Setup" };
    ScriptFunction<Scripting::Tensor()> train { "Train" };
    Scripting::NetworkState networkState { this };
};

struct ScriptTrainer : StrifeML::Trainer<ScriptNetwork>
{
    ScriptTrainer(ScriptResource* scriptResource)
        : StrifeML::Trainer<ScriptNetwork>(1, 1),
          scriptResource(scriptResource)
    {
        minSamplesBeforeStartingTraining = -1;
        script = scriptResource->CreateScript();
        script->TryCompile();   // TODO error checking
    }

    void RunBatch() override
    {
        if (script->TryRecompileIfNewer())
        {
            Log("Successfully recompiled\n");
            network->BindCallbacks(script, false);
        }

        Trainer<ScriptNetwork>::RunBatch();
    }

    bool TryCreateBatch(Grid<SampleType> outBatch) override
    {
        return true;
    }

    void OnCreateNewNetwork(std::shared_ptr<NetworkType> newNetwork)
    {
        newNetwork->BindCallbacks(script, true);
    }

    ScriptResource* scriptResource;
    std::shared_ptr<Script> script;
};

struct ScriptDecider : StrifeML::Decider<ScriptNetwork>
{

};