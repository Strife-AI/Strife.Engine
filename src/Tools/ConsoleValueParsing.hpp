#pragma once

#include <string>

enum class ConsoleValueParseResult
{
    Success,
    InvalidValue,
    InvalidRange
};

template<typename TVar>
ConsoleValueParseResult ParseConsoleValue(TVar& var, const std::string& str);

template<typename TVar>
std::string SerializeConsoleValue(const TVar& var);