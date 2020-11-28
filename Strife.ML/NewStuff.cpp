#include "NewStuff.hpp"

namespace StrifeML
{
    std::shared_ptr<MakeDecisionWorkItem> ScheduleDecision(
        std::shared_ptr<INeuralNetwork> network,
        std::shared_ptr<SerializedModel[]> input,
        int inputLength)
    {
        auto workItem = std::make_shared<MakeDecisionWorkItem>(network, input, inputLength);
        auto threadPool = ThreadPool::GetInstance();
        threadPool->StartItem(workItem);
        return workItem;
    }

    MakeDecisionWorkItem::MakeDecisionWorkItem(std::shared_ptr<INeuralNetwork> network_,
                                               std::shared_ptr<SerializedModel[]> input_, int inputLength_)
        : network(network_),
          input(input_),
          inputLength(inputLength_)
    {
    }

    void MakeDecisionWorkItem::Execute()
    {
        SerializedModel result;
        network->MakeDecision(gsl::span<SerializedModel>(input.get(), inputLength), result);
        _result = std::move(result);
    }
}
