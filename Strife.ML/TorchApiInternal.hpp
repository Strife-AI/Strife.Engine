#pragma once

#include "torch/torch.h"
#include "NewStuff.hpp"

struct TorchTensor
{
    TorchTensor()
    {

    }

    TorchTensor(const torch::IntArrayRef& ref)
        : tensor(torch::zeros(ref, torch::kFloat32))
    {

    }

    TorchTensor(const torch::Tensor& tensor)
        : tensor(tensor)
    {

    }

    torch::Tensor tensor;
};

struct Conv2D
{
    torch::nn::Conv2d conv2d { nullptr };
};

struct TensorDictionary
{
    void Add(const char* name, const torch::Tensor& tensor)
    {
        tensorsByName[name] = std::make_unique<TorchTensor>(tensor);
    }

    std::unordered_map<std::string, std::unique_ptr<TorchTensor>> tensorsByName;
};

struct TorchNetwork
{
    TorchNetwork(StrifeML::INeuralNetwork* network)
        : network(network)
    {

    }

    StrifeML::INeuralNetwork* network;
    std::unordered_map<std::string, std::unique_ptr<Conv2D>> conv2d;
    std::vector<std::unique_ptr<TorchTensor>> tensors;
};