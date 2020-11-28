#include "NewStuff.hpp"
#include "Scene/BaseEntity.hpp"

//struct MakeDecisionWorkItem : ThreadPoolWorkItem<SerializedModel>
//{
//
//
//    std::shared_ptr<INeuralNetwork> network;
//};
//
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