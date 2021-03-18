#pragma once

#include <vector>
#include <string>
#include <map>
#include "Thread/SpinLock.hpp"

struct Metric
{
    Metric(const std::string& name_)
        : name(name_)
    {

    }

    void Add(float value)
    {
        lock.Lock();
        data.push_back(value);
        lock.Unlock();
    }

    std::vector<float> data;
    std::string name;
    SpinLock lock;
};

class MetricsManager
{
public:
    ~MetricsManager();

    Metric* GetOrCreateMetric(const std::string& name);

private:
    std::map<std::string, Metric*> _metricsByName;
};

