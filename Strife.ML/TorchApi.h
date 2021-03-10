#ifndef TORCH_API_H
#define TORCH_API_H

#ifdef __cplusplus
namespace Scripting {
#endif

typedef struct Conv2D { int handle; } Conv2D;
typedef struct Tensor { int handle; } Tensor;

Tensor tensor_new();
Tensor tensor_new_4d(int x, int y, int z, int w);

Conv2D conv2d_add(const char* name, int a, int b, int c);
Conv2D conv2d_get(const char* name);
void conv2d_forward(Conv2D conv, Tensor input, Tensor output);

#ifdef __cplusplus
};
#endif

#endif
