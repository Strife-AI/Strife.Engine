#include "NewStuff.hpp"
#include "Scene/BaseEntity.hpp"

struct ChaserInput
{
    Vector2 velocity;
};

struct ChaserDecision
{
    
};

struct ChaserNetwork : INeuralNetwork<ChaserInput, ChaserDecision>
{

};

struct ChaserDecider : IDecider<ChaserNetwork>
{
    void MakeDecision(const InputType& input, OutputType& outDecision) override
    {
        
    }
};

struct ChaserTrainer : ITrainer<ChaserNetwork>
{
    
};

void Test()
{
    NeuralNetworkManager manager;
    auto decider = manager.CreateDecider<ChaserDecider>();
    auto trainer = manager.CreateTrainer<ChaserTrainer>();

    manager.CreateNetwork("blue", decider, trainer);
    manager.CreateNetwork("green", decider, trainer);
}

DEFINE_ENTITY(TestEntity, "test"), INeuralNetworkEntity<ChaserNetwork>
{
    void CollectData(InputType& outInput) override
    {
        outInput.velocity = { 10, 10 };
    }

    void ReceiveDecision(OutputType& output) override
    {
        
    }
};