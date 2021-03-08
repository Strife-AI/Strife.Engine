#ifndef TORCH_API_H
#define TORCH_API_H

typedef struct Conv2D Conv2D;
typedef struct TorchNetwork TorchNetwork;

Conv2D* network_conv2d_add(TorchNetwork* network, const char* name, int a, int b, int c);
Conv2D* network_conv2d_get(TorchNetwork* network, const char* name);

#endif