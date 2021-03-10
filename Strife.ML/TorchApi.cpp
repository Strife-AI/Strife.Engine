#include "TorchApi.h"
#include "TorchApiInternal.hpp"
#include "NewStuff.hpp"

namespace Scripting
{

thread_local ScriptingState g_scriptState;

ScriptingState* GetScriptingState()
{
    return &g_scriptState;
}

NetworkState* GetNetwork() { return g_scriptState.network; }

#define NOT_NULL(name_) { if (name_ == nullptr) throw StrifeML::StrifeException("Paramater " + std::string(#name_) + " is NULL"); }


Conv2D conv2d_add(const char* name, int a, int b, int c)
{
    NOT_NULL(name);

    auto network = GetNetwork();
    auto [conv, handle] = network->conv2d.Create(name);
    conv->conv2d = network->network->register_module(name, torch::nn::Conv2d { a, b, c });
    return handle;
}

Conv2D conv2d_get(const char* name)
{
    NOT_NULL(name);
    return GetNetwork()->conv2d.GetHandleByName(name);
}

#define CONV2D_MEMBER_FUNCTION(name_, memberFunction_) void name_(Conv2D conv, Tensor input, Tensor output) \
{                                                                                                           \
    auto convImpl = GetNetwork()->conv2d.Get(conv);                                                         \
    auto tensorInput = g_scriptState.tensors.Get(input);                                                    \
    auto tensorOutput = g_scriptState.tensors.Get(output);                                                  \
    tensorOutput->tensor = convImpl->conv2d->memberFunction_(tensorInput->tensor);                                  \
}

CONV2D_MEMBER_FUNCTION(conv2d_forward, forward)

Tensor tensor_new()
{
    auto [obj, handle] = g_scriptState.tensors.Create();
    return handle;
}

Tensor tensor_new_4d(int x, int y, int z, int w)
{
    auto [obj, handle] = g_scriptState.tensors.Create(torch::IntArrayRef { x, y, z, w });
    return handle;
}

}