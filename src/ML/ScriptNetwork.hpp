#pragma once

#include <variant>
#include <torch/torch.h>
#include <Strife.ML/TorchApi.h>
#include "ML.hpp"
#include "Scripting/Scripting.hpp"
#include "Strife.ML/TorchApiInternal.hpp"
#include "Resource/ScriptResource.hpp"


template<typename TInput, typename TOutput>
struct ScriptNetwork : StrifeML::NeuralNetwork<TInput, TOutput, 1>
{
    using SampleType = StrifeML::Sample<TInput, TOutput>;

    void MakeDecision(Grid<const TInput> input, TOutput& output) override
    {

    }

    void TrainBatch(Grid<const SampleType> input, StrifeML::TrainingBatchResult& outResult) override
    {
        VariableSizedGrid<Scripting::SerializedInput> serializedInput(input.Rows(), input.Cols());

        for (int i = 0; i < input.Rows(); ++i)
        {
            for (int j = 0; j < input.Cols(); ++j)
            {
                // Safe to remove constness because we're in writing mode
                auto inputElement = const_cast<SampleType&>(input[i][j]);
                serializedInput[i][j].serializer.isReading = false;
                inputElement.input.Serialize(serializedInput[i][j].serializer);
                inputElement.output.Serialize(serializedInput[i][j].serializer);
            }
        }

        networkState.input = serializedInput;

        try
        {
            DoScriptCall([=] { train(); });
        }
        catch (...)
        {
            // TODO
        }

        networkState.input.Set(0, 0, nullptr);
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

template<typename TInput, typename TOutput>
struct ScriptTrainer : StrifeML::Trainer<ScriptNetwork<TInput, TOutput>>
{
    using SampleType = StrifeML::Sample<TInput, TOutput>;
    using NetworkType = ScriptNetwork<TInput, TOutput>;

    ScriptTrainer(ScriptResource* scriptResource)
        : StrifeML::Trainer<ScriptNetwork<TInput, TOutput>>(1, 1),
          scriptResource(scriptResource)
    {
        this->minSamplesBeforeStartingTraining = -1;
        script = scriptResource->CreateScript();
        script->TryCompile();   // TODO error checking
    }

    void RunBatch() override
    {
        if (script->TryRecompileIfNewer())
        {
            Log("Successfully recompiled\n");
            this->network->BindCallbacks(script, false);
        }

        StrifeML::Trainer<NetworkType>::RunBatch();
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

template<typename TInput, typename TOutput>
struct ScriptDecider : StrifeML::Decider<ScriptNetwork<TInput, TOutput>>
{

};