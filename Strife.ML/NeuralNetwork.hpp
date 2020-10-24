#pragma once

#include <torch/torch.h>

struct NeuralNetwork : torch::nn::Module
{
	NeuralNetwork();

    torch::Tensor forward(const torch::Tensor& spatialInput, const torch::Tensor& featureInput);

	torch::nn::Embedding embedding{ nullptr };
	torch::nn::Conv2d conv1{ nullptr }, conv2{ nullptr }, conv3{ nullptr }, conv4{ nullptr };//, conv5{ nullptr };
	//torch::nn::BatchNorm2d batchNorm1{ nullptr }, batchNorm2{ nullptr }, batchNorm3{ nullptr }, batchNorm4{ nullptr }, batchNorm5{ nullptr };
	torch::nn::Linear dense{ nullptr };
	//torch::nn::LSTM lstm{ nullptr };

    std::shared_ptr<torch::optim::Adam> optimizer;
};