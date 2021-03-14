#ifndef TORCH_API_H
#define TORCH_API_H

#ifdef __cplusplus
namespace Scripting {
#endif

typedef struct Conv2D { int handle; } Conv2D;
typedef struct Tensor { int handle; } Tensor;
typedef struct LinearLayer { int handle; } LinearLayer;
typedef struct Input { int handle; } Input;

Tensor tensor_new();
Tensor tensor_new_4d(int x, int y, int z, int w);
Tensor tensor_clone(Tensor input);

Conv2D conv2d_add(const char* name, int a, int b, int c);
Conv2D conv2d_get(const char* name);
void conv2d_forward(Conv2D conv, Tensor input, Tensor output);

LinearLayer linearlayer_add(const char* name, int totalFeatures, int hiddenNodesCount);
LinearLayer linearlayer_get(const char* name);
void linearlayer_forward(LinearLayer layer, Tensor input);

void relu(Tensor input);

Tensor pack_float_array(const char* attributeNames[], int count);

#ifdef __cplusplus
};
#endif

#endif
