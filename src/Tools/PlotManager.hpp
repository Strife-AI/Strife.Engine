#pragma once

#include <vector>

#include "Memory/Singleton.hpp"
#include "ConsoleCmd.hpp"

struct Metric;
class ConsoleCommandBinder;

struct PlotInstance
{
    PlotInstance(Metric* metric_, int x_, int y_)
        : metric(metric_),
        x(x_),
        y(y_)
    {

    }

    bool isDestroyed = false;
    Metric* metric;
    int x;
    int y;
};

class PlotManager
{
public:
    PlotManager()
        : _plotCommand("plot", PlotCommand)
    {

    }

    ~PlotManager();

    void RenderPlots();

private:
    static void PlotCommand(ConsoleCommandBinder& binder);
    ConsoleCmd _plotCommand;

    std::vector<PlotInstance*> _openPlots;
};

