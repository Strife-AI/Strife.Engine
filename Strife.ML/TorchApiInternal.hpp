#pragma once

#include "torch/torch.h"
#include "NewStuff.hpp"

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

struct Conv2D
{
    torch::nn::Conv2d conv2d { nullptr };
};


struct TorchContext
{
    std::vector<std::unique_ptr<Tensor>> tensors;
};

struct TorchNetwork
{
    StrifeML::INeuralNetwork* network;
    std::unordered_map<std::string, std::unique_ptr<Conv2D>> conv2d;
};