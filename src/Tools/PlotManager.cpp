#include <imgui.h>

#include "Engine.hpp"
#include "PlotManager.hpp"
#include "MetricsManager.hpp"
#include "SdlManager.hpp"

void PlotManager::PlotCommand(ConsoleCommandBinder& binder)
{
    std::string metricName;
    int screenX;
    int screenY;

    binder
        .Bind(metricName, "metric")
        .Bind(screenX, "screenX")
        .Bind(screenY, "screenY")
        .Help("Creates a line plot");

    auto engine = Engine::GetInstance();
    auto& openPlots = engine->GetPlotManager()->_openPlots;
    for(auto plot : openPlots)
    {
        if(plot->metric->name == metricName)
        {
            return;
        }
    }

    auto metric = engine->GetMetricsManager()->GetOrCreateMetric(metricName);
    openPlots.push_back(new PlotInstance(metric, screenX, screenY));
}


void PlotManager::RenderPlots()
{
    for(int i = (int)_openPlots.size() - 1; i >= 0; --i)
    {
        PlotInstance* plot = _openPlots[i];
        auto& metric = plot->metric;

        if(plot->isDestroyed)
        {
            _openPlots.erase(_openPlots.begin() + i);
            delete plot;
            continue;
        }

        metric->lock.Lock();
        {
            auto& data = metric->data;
            int dataSize = data.size();
            bool isOpen = true;

            ImGui::SetNextWindowPos(ImVec2(plot->x, plot->y), ImGuiCond_Once);

            auto dpiRatio = Engine::GetInstance()->GetSdlManager()->DpiRatio();
            ImGui::SetNextWindowSize(ImVec2(300 * dpiRatio, 150 * dpiRatio), ImGuiCond_Once);

            ImGui::Begin(plot->metric->name.c_str(), &isOpen);

            char currentValue[32];
            snprintf(currentValue, sizeof(currentValue), "%f", dataSize > 0 ? data[dataSize - 1] : 0);

            int count = Min(dataSize, 50);

            auto size = ImGui::GetWindowSize();

            ImGui::PlotLines(plot->metric->name.c_str(), &data.data()[dataSize - count], count, 0, currentValue, FLT_MAX, FLT_MAX, ImVec2(size.x - 150, size.y - 100));
            ImGui::End();

            if(!isOpen)
            {
                plot->isDestroyed = true;
            }
        }
        metric->lock.Unlock();
    }
}

PlotManager::~PlotManager()
{
    for(auto plot :_openPlots)
    {
        delete plot;
    }
}
