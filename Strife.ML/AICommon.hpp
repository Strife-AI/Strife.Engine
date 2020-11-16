#pragma once

#include <sstream>
#include <memory>
#include <cassert>

#include "Memory/Grid.hpp"
#include "Math/Vector2.hpp"
#include "Memory/ConcurrentQueue.hpp"

const int PerceptionGridRows = 40;
const int PerceptionGridCols = 40;
const int PerceptionGridCells = PerceptionGridRows * PerceptionGridCols;
const int SequenceLength = 1;
const int InputFeaturesCount = 2;

using PerceptionGridType = long long;
using PerceptionGrid = FixedSizeGrid<PerceptionGridType, PerceptionGridRows, PerceptionGridCols>;

enum class CharacterAction;
class Scene;

class CompressedPerceptionGridRectangle
{
public:
	static const int ObservedObjectStartBit = 0;
	static const int ObservedObjectBits = 4;

	static const int DimensionBits = 7;
	static const int XStartBit = ObservedObjectStartBit + ObservedObjectBits;
	static const int YStartBit = XStartBit + DimensionBits;
	static const int WidthStartBit = YStartBit + DimensionBits;
	static const int HeightStartBit = WidthStartBit + DimensionBits;

	CompressedPerceptionGridRectangle(int type, int x, int y, int width, int height)
	{
		_value = IncludeValue(type, ObservedObjectStartBit, ObservedObjectBits)
			| IncludeValue(x, XStartBit, DimensionBits)
			| IncludeValue(y, YStartBit, DimensionBits)
			| IncludeValue(width, WidthStartBit, DimensionBits)
			| IncludeValue(height, HeightStartBit, DimensionBits);
	}

	CompressedPerceptionGridRectangle(unsigned int value)
		: _value(value)
	{

	}

	CompressedPerceptionGridRectangle()
	{

	}

	int ObservedObject() const { return GetValue(ObservedObjectStartBit, ObservedObjectBits); }
	int X() const { return GetValue(XStartBit, DimensionBits); }
	int Y() const { return GetValue(YStartBit, DimensionBits); }
	int Width() const { return GetValue(WidthStartBit, DimensionBits); }
	int Height() const { return GetValue(HeightStartBit, DimensionBits); }

	unsigned int Data() const
	{
		return _value;
	}

private:
	int GetValue(const int startBit, const int totalBits) const
	{
		return static_cast<int>((_value >> startBit) & ((1 << totalBits) - 1));
	}

	static unsigned int IncludeValue(int value, int startBit, int totalBits)
	{
		assert(value < (1 << totalBits));

		return value << startBit;
	}

	unsigned int _value;
};


struct Sample
{
    std::vector<CompressedPerceptionGridRectangle> compressedRectangles;
    PerceptionGrid grid;
    CharacterAction action;
    Vector2 velocity;
	Vector2 center;
};

struct DecompressedExperience
{
    DecompressedExperience(PerceptionGridType* data)
        : perceptionGrid(PerceptionGridRows, PerceptionGridCols, data)
    {

    }

    DecompressedExperience() = default;

    DecompressedExperience(const DecompressedExperience&) = delete;

    void SetData(PerceptionGridType* data)
    {
        perceptionGrid.Set(PerceptionGridCols, PerceptionGridRows, data);
    }

    void CopyTo(DecompressedExperience& outExperience) const
    {
        perceptionGrid.FastCopyUnsafe(outExperience.perceptionGrid);
        outExperience.velocity = velocity;
    }

    Grid<PerceptionGridType> perceptionGrid;
    Vector2 velocity;
};

struct Model
{
    Model() = default;
    Model(std::shared_ptr<std::stringstream> stream_)
        : stream(stream_)
    {
        
    }

    std::shared_ptr<std::stringstream> stream;
};

struct ModelBinding
{
    ModelBinding() = default;
    ModelBinding(std::shared_ptr<ConcurrentQueue<Model>> channel)
        : communicationChannel(channel)
    {
        
    }

    std::shared_ptr<ConcurrentQueue<Model>> communicationChannel;
};