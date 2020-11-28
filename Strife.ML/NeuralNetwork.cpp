#include "NeuralNetwork.hpp"

#include "AICommon.hpp"
#include "CharacterAction.hpp"
#include "ObservedObject.hpp"

#define DEBUG 1

#if DEBUG
#define DEBUG_TRAINER(statement_) if (is_training()) { std::cout << statement_ << std::endl; }
#else
#define DEBUG_TRAINER(...) ;
#endif

OldNeuralNetwork::OldNeuralNetwork()
{
    //torch::nn::functional::detail::embedding = register_module("embedding", torch::nn::Embedding(static_cast<int>(ObservedObject::TotalObjects), 4));
	conv1 = register_module("conv1", torch::nn::Conv2d(4, 8, 5));
	//batchNorm1 = register_module("batchNorm1", torch::nn::BatchNorm2d(8));
	conv2 = register_module("conv2", torch::nn::Conv2d(8, 16, 3));
	//batchNorm2 = register_module("batchNorm2", torch::nn::BatchNorm2d(16));
	conv3 = register_module("conv3", torch::nn::Conv2d(16, 32, 3));
    //batchNorm3 = register_module("batchNorm3", torch::nn::BatchNorm2d(32));
	conv4 = register_module("conv4", torch::nn::Conv2d(32, 64, 3));
	//batchNorm4 = register_module("batchNorm4", torch::nn::BatchNorm2d(64));
	//conv5 = register_module("conv5", torch::nn::Conv2d(64, 128, 3));
	//batchNorm5 = register_module("batchNorm5", torch::nn::BatchNorm2d(128));
    //lstm = register_module("lstm", torch::nn::LSTM(torch::nn::LSTMOptions(128, 128)));
	dense = register_module("dense", torch::nn::Linear(64, static_cast<int>(CharacterAction::TotalActions)));
}

torch::Tensor OldNeuralNetwork::forward(const torch::Tensor& spatialInput, const torch::Tensor& featureInput)
{
    //assert(spatialInput.size(0) == featureInput.size(0)); // Sequence
    //assert(spatialInput.size(1) == featureInput.size(1)); // Batch
	
    // spatialInput's shape: S x B x Rows x Cols
    auto sequenceLength = spatialInput.size(0);
    auto batchSize = spatialInput.size(1);
    auto height = spatialInput.size(2);
    auto width = spatialInput.size(3);

    torch::Tensor x;

	if (sequenceLength > 1)
	{
		x = spatialInput.view({ -1, height, width });
	}
	else
	{
		x = torch::squeeze(spatialInput, 0);
	}

    x = embedding->forward(x);     // N x 80 x 80 x 4
    x = x.permute({ 0, 3, 1, 2 }); // N x 4 x 80 x 80

    x = torch::leaky_relu(conv1->forward(x)); // N x 8 x 76 x 76
    //x = batchNorm1->forward(x);
    x = torch::dropout(x, 0.5, is_training());
    x = torch::max_pool2d(x, { 2, 2 }); // N x 8 x 38 x 38

    x = torch::leaky_relu(conv2->forward(x)); // N x 16 x 36 x 36
    //x = batchNorm2->forward(x);
    x = torch::dropout(x, 0.5, is_training());
    x = torch::max_pool2d(x, { 2, 2 }); // N x 16 x 18 x 18

    x = torch::leaky_relu(conv3->forward(x)); // N x 32 x 16 x 16
    //x = batchNorm3->forward(x);
    x = torch::dropout(x, 0.5, is_training());
    x = torch::max_pool2d(x, { 2, 2 }); // N x 32 x 8 x 8

    x = torch::leaky_relu(conv4->forward(x)); // N x 64 x 6 x 6
    //x = batchNorm4->forward(x);
    x = torch::dropout(x, 0.5, is_training());
	//x = torch::max_pool2d(x, { 2, 2 }); // N x 32 x 3 x 3

	//x = torch::leaky_relu(conv5->forward(x)); // N x 64 x 1 x 1
    //x = batchNorm5->forward(x);
    //x = torch::dropout(x, 0.5, is_training());

    if (sequenceLength > 1)
    {
	    x = x.view({sequenceLength, batchSize, 128});
    }
    else
    {
        x = x.view({batchSize, 64});
    }

	//if (InputFeaturesCount > 0)
    //{
	//x = torch::cat({x, featureInput}, -1); // -1 means concat on last dimension
    //}

	//if (sequenceLength > 1) {
	//	auto lstmOut = lstm->forward(x);
	//	x = std::get<0>(lstmOut);
	//	x = torch::narrow(x, 0, SequenceLength - 1, 1);
	//	x = torch::squeeze(x, 0);
    //}
	x = dense->forward(x);
    x = torch::log_softmax(x, 1);

    return x;
}
