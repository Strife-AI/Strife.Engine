#pragma once
#include <map>
#include <memory>
#include <string>

#include "Memory/Dictionary.hpp"

class UiDictionary;

struct UiDictionaryProperty
{
    UiDictionaryProperty(const std::string& value_)
        : value(value_)
    {
        
    }

    UiDictionaryProperty(std::unique_ptr<UiDictionary>& dictionary_)
        : dictionary(std::move(dictionary_))
    {
        
    }

    std::string value;
    std::unique_ptr<UiDictionary> dictionary = nullptr;
};

class UiDictionary : public Dictionary<UiDictionary>
{
public:
    void AddProperty(const char* key, std::string& value);
    void AddProperty(const char* key, std::unique_ptr<UiDictionary>& dictionary);

    void Print()
    {
        printf("[Dictionary]\n");
        PrintRecursive(0);
    }

    bool HasProperty(const char* key) const override
    {
        return _properties.count(key) != 0;
    }

private:
    template<typename T>
    bool TryGetCustomProperty(const char* key, T& outResult) const
    {
        return false;
    }

    bool TryGetStringViewProperty(const char* key, std::string_view& result) const
    {
        UiDictionaryProperty* property;
        if(TryGetPropertyInternal(key, property) && property->dictionary == nullptr)
        {
            result = property->value;
            return true;
        }
        else
        {
            return false;
        }
    }

    bool TryGetPropertyInternal(const char* key, UiDictionaryProperty*& outProperty) const
    {
        auto findResult = _properties.find(std::string(key));

        if (findResult == _properties.end())
        {
            return false;
        }
        else
        {
            outProperty = findResult->second.get();
            return true;
        }
    }

    void PrintRecursive(int indent);

    friend class Dictionary<UiDictionary>;

    std::map<std::string, std::unique_ptr<UiDictionaryProperty>> _properties;
};

inline void UiDictionary::AddProperty(const char* key, std::string& value)
{
    _properties[std::string(key)] = std::make_unique<UiDictionaryProperty>(value);
}

inline void UiDictionary::AddProperty(const char* key, std::unique_ptr<UiDictionary>& dictionary)
{
    _properties[std::string(key)] = std::make_unique<UiDictionaryProperty>(dictionary);
}

inline void UiDictionary::PrintRecursive(int indent)
{
    for(auto& p : _properties)
    {
        for (int i = 0; i < indent; ++i) printf("\t");

        if(p.second->dictionary != nullptr)
        {
            printf("%s: [dictionary]\n", p.first.c_str());
            p.second->dictionary->PrintRecursive(indent + 1);
        }
        else
        {
            printf("'%s': '%s'\n", p.first.c_str(), p.second->value.c_str());
        }
    }
}

template<>
bool UiDictionary::TryGetCustomProperty(const char* key, UiDictionary*& result) const
{
    UiDictionaryProperty* property;
    if (TryGetPropertyInternal(key, property) && property->dictionary != nullptr)
    {
        result = property->dictionary.get();
        return true;
    }
    else
    {
        return false;
    }
}
