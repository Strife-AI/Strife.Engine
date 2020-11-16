#pragma once

#include "Memory/Grid.hpp"
#include <cassert>
#include <Memory/SpinLock.hpp>
#include <random>
#include <Renderer/Color.hpp>
#include "Math/Vector2.hpp"
#include "Math/Rectangle.hpp"
#include "ObservedObject.hpp"
#include "AICommon.hpp"
#include "Physics/Physics.hpp"
#include "CharacterAction.hpp"
#include "gsl/span"
#include "Memory/EnumDictionary.hpp"
#include "Memory/FixedSizeVector.hpp"

class Scene;
class BinaryStreamReader;
class BinaryStreamWriter;
struct DecompressedExperience;
struct ExperienceRequest;
class Renderer;

constexpr float PerceptionGridCellSize = 8;

constexpr int TotalColumnWidth()
{
	return (PerceptionGridCellSize * PerceptionGridCols);
}

constexpr int TotalColumnHeight()
{
	return (PerceptionGridCellSize * PerceptionGridRows);
}

struct CompressedExperience
{
	CompressedExperience(int perceptionGridDataStartIndex_, Vector2 velocity_)
		: perceptionGridDataStartIndex(perceptionGridDataStartIndex_),
	    velocity(velocity_)
	{

	}

	CompressedExperience() = default;

	int perceptionGridDataStartIndex;
	Vector2 velocity;
};

struct PerceptionGridRectangle
{
	PerceptionGridRectangle(const Rectanglei& rect, ObservedObject observedObject_)
		: rectangle(rect),
		observedObject(observedObject_)
	{

	}

	PerceptionGridRectangle() = default;

	Rectanglei rectangle;
	ObservedObject observedObject;
};

class ExperienceManager
{
public:
	static const int MaxExperiences = 100000;
	static const int PerceptionGridDataSize = MaxExperiences * 8;

	ExperienceManager();

	// Returns an experience id
	int CreateExperience(const std::vector<CompressedPerceptionGridRectangle>& rectangles, Vector2 velocity);

	void DecompressPerceptionGrid(const CompressedExperience* experience, Grid<long long>& outGrid) const;
	void DecompressExperience(int experienceId, DecompressedExperience& outExperience) const;
    void RenderExperience(int experienceId, Renderer* renderer, Vector2 topLeft, float scale = 1);

    static void SamplePerceptionGrid(Scene* scene, const Vector2 topLeftPosition, Grid<long long>& outGrid);
	static void SamplePerceptionGrid(Scene* scene, const Vector2 topLeftPosition, std::vector<CompressedPerceptionGridRectangle>& outRectangles);

    static void RenderPerceptionGrid(Grid<long long>& grid, const Vector2 topLeftPosition, Renderer* renderer, float scale = 1);

	void DiscardExperiencesBefore(int experienceId);
	int TotalExperiences() const { return _nextFreeExperience; }

	void Serialize(BinaryStreamWriter& writer) const;
	int Deserialize(BinaryStreamReader& reader);

private:
	static int TotalPerceptionGridRectangles(const CompressedExperience* experience);
	static PerceptionGridRectangle DecompressRectangle(const CompressedPerceptionGridRectangle& compressedRect);
	static void FillGridWithRectangle(Grid<long long>& grid, const PerceptionGridRectangle& rect);

    static gsl::span<PerceptionGridRectangle> GetPerceptionGridRectangles(
		Scene* scene,
		Vector2 topLeftPosition,
		gsl::span<PerceptionGridRectangle> storage);

	void FillColliders(Grid<long long>& outGrid, const CompressedExperience* const experience) const;

	CompressedPerceptionGridRectangle GetCompressedGridRectangle(int rectangleId) const;
	const CompressedExperience* GetExperience(int experienceId) const;
	CompressedExperience* GetExperience(int experienceId);

	unsigned int _compressedPerceptionGridData[PerceptionGridDataSize];
	CompressedExperience _experiences[MaxExperiences];

	int _nextFreePerceptionGridDataIndex = 0;
	int _nextFreeExperience = 0;
	SpinLock _createLock;

    static Color _objectColors[static_cast<int>(ObservedObject::TotalObjects)];
};

struct SampleManager
{
    void Reset();
	void Serialize(BinaryStreamWriter& writer);
	void Deserialize(BinaryStreamReader& reader);

    bool TryGetRandomSample(CharacterAction type, gsl::span<DecompressedExperience> outExperiences);
	void AddSample(int experienceId, CharacterAction action);
	bool HasSamples(CharacterAction action);
	bool HasSamples();

	ExperienceManager experienceManager;
	EnumDictionary<
		CharacterAction,
		FixedSizeVector<int, ExperienceManager::MaxExperiences>,
		(int)CharacterAction::TotalActions> experiencesByActionType;

	std::default_random_engine generator;
};

constexpr int TotalColumnHeight();