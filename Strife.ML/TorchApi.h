#ifndef TORCH_API_H
#define TORCH_API_H

typedef struct Conv2D Conv2D;
typedef struct TorchNetwork TorchNetwork;
typedef struct TorchTensor TorchTensor;
typedef struct TensorDictionary TensorDictionary;

TorchTensor* tensor_new(TorchNetwork* network);
TorchTensor* tensor_new_4d(TorchNetwork* network, int x, int y, int z, int w);

Conv2D* network_conv2d_add(TorchNetwork* network, const char* name, int a, int b, int c);
Conv2D* network_conv2d_get(TorchNetwork* network, const char* name);
void conv2d_forward(Conv2D* conv, TorchTensor* input, TorchTensor* output);

TorchTensor* tensordictionary_get(TensorDictionary* dictionary, const char* name);

#endif