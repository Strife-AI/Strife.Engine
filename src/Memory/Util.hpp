#pragma once

#include <vector>
#include <algorithm>

template<typename T>
void RemoveFromVector(std::vector<T>& v, const T& element)
{
    v.erase(std::remove(v.begin(), v.end(), element), v.end());
}

