#pragma once

#include <functional>
#include <map>
#include <vector>

template<typename TState>
struct UtilityConsideration
{
    std::string name;
    std::function<float(const TState& state)> evaluate;
};

using UtilityConsiderationSet = std::map<std::string, float>;

template<typename TState>
struct UtilityAction
{
    std::string name;
    std::function<float(UtilityConsiderationSet& set)> evaluate;
};

template<typename TState>
struct UtilityAI
{
    void AddAction(const std::string& name, const std::function<float(UtilityConsiderationSet& set)>& evaluate)
    {
        UtilityAction<TState> action;
        action.name = name;
        action.evaluate = evaluate;
        actions.push_back(action);
    }

    void AddConsideration(const std::string& name, const std::function<float(const TState& state)>& evaluate)
    {
        UtilityConsideration<TState> consideration;
        consideration.name = name;
        consideration.evaluate = evaluate;
        considerations.push_back(consideration);
    }

    void Evaluate(const TState& state, std::vector<float>& outUtility) const
    {
        UtilityConsiderationSet set;
        for (auto& consideration : considerations)
        {
            set[consideration.name] = consideration.evaluate(state);
        }

        for (int i = 0; i < actions.size(); ++i)
        {
            outUtility.push_back(actions[i].evaluate(set));
        }
    }

    int PickBestAction(const TState& state) const
    {
        std::vector<float> utility;
        Evaluate(state, utility);

        int maxIndex = 0;
        for (int i = 1; i < utility.size(); ++i)
        {
            if (utility[i] > utility[maxIndex]) maxIndex = i;
        }

        return maxIndex;
    }

    std::vector<UtilityAction<TState>> actions;
    std::vector<UtilityConsideration<TState>> considerations;
};