#pragma once

#include "ML.hpp"

struct Tensor
{
    Tensor()
    {

    }

    Tensor(const torch::IntArrayRef& ref)
        : tensor(torch::zeros(ref, torch::kFloat32))
    {

    }

    Tensor(const torch::Tensor& tensor)
        : tensor(tensor)
    {

    }

    torch::Tensor tensor;
};

struct TorchContext
{
    std::vector<std::unique_ptr<Tensor>> tensors;
};



template<typename TNetwork>
struct ScriptNetwork : NeuralNetwork<TNetwork>
{
    void (*train)(TorchContext* torchContext);
};