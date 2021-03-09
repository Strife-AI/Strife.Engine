#include "TorchApi.h"
#include "TorchApiInternal.hpp"
#include "NewStuff.hpp"

#define NOT_NULL(name_) { if (name_ == nullptr) throw StrifeML::StrifeException("Paramater " + std::string(#name_) + " is NULL"); }

Conv2D* network_conv2d_add(TorchNetwork* network, const char* name, int a, int b, int c)
{
    NOT_NULL(network); NOT_NULL(name);

    auto conv = new Conv2D;
    network->conv2d[name] = std::unique_ptr<Conv2D>(conv);
    conv->conv2d = network->network->register_module(name, torch::nn::Conv2d { a, b, c });
    return conv;
}

Conv2D* network_conv2d_get(TorchNetwork* network, const char* name)
{
    NOT_NULL(network); NOT_NULL(name);

    auto it = network->conv2d.find(name);
    if (it != network->conv2d.end())
    {
        return it->second.get();
    }
    else
    {
        throw StrifeML::StrifeException("Missing conv2d: " + std::string(name));
    }
}

void conv2d_forward(Conv2D* conv, TorchTensor* input, TorchTensor* output)
{
    NOT_NULL(conv); NOT_NULL(input); NOT_NULL(output);
    output->tensor = conv->conv2d->forward(input->tensor);
}

TorchTensor* tensor_new(TorchNetwork* network)
{
    NOT_NULL(network);

    auto tensor = new TorchTensor;
    network->tensors.push_back(std::unique_ptr<TorchTensor>(tensor));
    return tensor;
}

TorchTensor* tensor_new_4d(TorchNetwork* network, int x, int y, int z, int w)
{
    NOT_NULL(network);

    auto tensor = new TorchTensor(torch::IntArrayRef { x, y, z, w });
    network->tensors.push_back(std::unique_ptr<TorchTensor>(tensor));
    return tensor;
}

TorchTensor* tensordictionary_get(TensorDictionary* dictionary, const char* name)
{
    NOT_NULL(dictionary); NOT_NULL(name);

    auto it = dictionary->tensorsByName.find(name);
    if (it != dictionary->tensorsByName.end())
    {
        return it->second.get();
    }
    else
    {
        throw StrifeML::StrifeException("Missing tensor in dictionary: " + std::string(name));
    }
}
