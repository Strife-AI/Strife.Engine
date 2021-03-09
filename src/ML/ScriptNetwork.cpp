#include "ScriptNetwork.hpp"
#include "Strife.ML/TorchApi.h"

void RegisterScriptFunctions()
{
    SCRIPT_REGISTER(tensor_new);
    SCRIPT_REGISTER(tensor_new_4d);

    SCRIPT_REGISTER(network_conv2d_add)
    SCRIPT_REGISTER(network_conv2d_get)
    SCRIPT_REGISTER(conv2d_forward);

    SCRIPT_REGISTER(tensordictionary_get);
};