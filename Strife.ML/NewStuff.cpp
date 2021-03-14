#include "NewStuff.hpp"
#include "torch/nn/module.h"
#include "torch/serialize.h"

namespace StrifeML
{
    void ObjectSerializer::AddBytes(unsigned char* data, int size)
    {
        if (hadError)
        {
            return;
        }

        if (isReading)
        {
            int i;
            for (i = 0; i < size && readOffset < bytes.size(); ++i)
            {
                data[i] = bytes[readOffset++];
            }

            if (i != size)
            {
                // Ran out of bytes
                hadError = true;
            }
        }
        else
        {
            for (int i = 0; i < size; ++i)
            {
                bytes.push_back(data[i]);
            }
        }
    }

    std::shared_ptr<torch::nn::Module> CreateModule()
    {
        return std::make_shared<torch::nn::Module>();
    }

    void TorchLoad(std::shared_ptr<torch::nn::Module> module, std::stringstream& stream)
    {
        torch::load(module, stream);
    }

    void TorchSave(std::shared_ptr<torch::nn::Module> module, std::stringstream& stream)
    {
        torch::save(module, stream);
    }
}
