#include "MetricsManager.hpp"

Metric* MetricsManager::GetOrCreateMetric(const std::string& name)
{
    auto metric = _metricsByName.find(name);

    if(metric != _metricsByName.end())
    {
        return metric->second;
    }
    else
    {
        auto metric = new Metric(name);
        _metricsByName[name] = metric;
        return metric;
    }
}

MetricsManager::~MetricsManager()
{
    for(auto& metric : _metricsByName)
    {
        delete metric.second;
    }
}
