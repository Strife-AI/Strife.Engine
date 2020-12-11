#pragma once

#include <string>
#include <vector>

inline std::string StripWhitespace(const std::string& str)
{
    std::string output;
    output.reserve(str.size());

    for(auto c : str)
    {
        if(!isspace(c))
        {
            output.push_back(c);
        }
    }

    return output;
}

inline std::vector<std::string> SplitString(const std::string& str, char split)
{
    std::vector<std::string> result;
    std::string token;

    for(auto c : str)
    {
        if(c == split)
        {
            result.push_back(token);
            token.clear();
        }
        else
        {
            token.push_back(c);
        }
    }

    result.push_back(token);

    return result;
}