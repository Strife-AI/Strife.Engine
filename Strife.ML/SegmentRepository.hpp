#pragma once
#include <string>
#include <unordered_map>

struct SegmentInfo
{
    int sampleCount = 0;
    bool hasBeenPlayed = false;
};

class SegmentRepository
{
public:
    int TotalSegmentSamples(const std::string& segmentName) const
    {
        auto segment = _segmentInfoByName.find(segmentName);
        if (segment != _segmentInfoByName.end())
        {
            return segment->second.sampleCount;
        }
        else
        {
            return 0;
        }
    }

    bool SegmentHasBeenPlayed(const std::string& segmentName) const
    {
        auto segment = _segmentInfoByName.find(segmentName);
        if (segment != _segmentInfoByName.end())
        {
            return segment->second.hasBeenPlayed;
        }
        else
        {
            return false;
        }
    }

    void SetSampleCount(const std::string& segmentName, int totalSamples)
    {
        auto& segment = _segmentInfoByName[segmentName];
        segment.sampleCount = totalSamples;
        segment.hasBeenPlayed = true;
    }

private:
    std::unordered_map<std::string, SegmentInfo> _segmentInfoByName;
};