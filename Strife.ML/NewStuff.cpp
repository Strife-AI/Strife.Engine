#include "NewStuff.hpp"

using namespace StrifeML;

struct MakeDecisionWorkItem : ThreadPoolWorkItem<SerializedModel>
{
    void Execute() override
    {
        SerializedModel result;
        network->MakeDecision(gsl::span<SerializedModel>(input.get(), inputLength), result);
        _result = std::move(result);
    }

    std::shared_ptr<INeuralNetwork> network;
    std::shared_ptr<SerializedModel[]> input;
    int inputLength;
};

//struct NewDecider
//{
//    std::shared_ptr<MakeDecisionWorkItem> MakeDecision(const std::shared_ptr<InputType[]>& input, int inputSize)
//    {
//        auto threadPool = ThreadPool::GetInstance();
//        auto workItem = std::make_shared<MakeDecisionWorkItem>(network, input, inputSize);
//        threadPool->StartItem(workItem);
//        return workItem;
//    }
//
//    std::shared_ptr<INeuralNetwork> network;
//};