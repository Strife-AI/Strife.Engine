#include "TorchApi.h"
#include "TorchApiInternal.hpp"
#include "NewStuff.hpp"

Conv2D* network_conv2d_add(TorchNetwork* network, const char* name, int a, int b, int c)
{
    auto conv = new Conv2D;
    network->conv2d[name] = std::unique_ptr<Conv2D>(conv);
    conv->conv2d = network->network->register_module(name, torch::nn::Conv2d { a, b, c });
    return conv;
}

Conv2D* network_conv2d_get(TorchNetwork* network, const char* name)
{
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