#include "ExperienceManager.hpp"

#include <iostream>

#include "Renderer/SdlExtensions.hpp"
#include "Scene/Scene.hpp"
#include "Renderer/Renderer.hpp"
#include "AICommon.hpp"
#include "System/BinaryStreamReader.hpp"
#include "System/BinaryStreamWriter.hpp"

Color ExperienceManager::_objectColors[static_cast<int>(ObservedObject::TotalObjects)];

ExperienceManager::ExperienceManager()
{
	_objectColors[static_cast<int>(ObservedObject::Nothing)] = Color::Black();
	_objectColors[static_cast<int>(ObservedObject::Wall)] = Color::Gray();
	_objectColors[static_cast<int>(ObservedObject::Breakable)] = Color::Brown();
	_objectColors[static_cast<int>(ObservedObject::GravityWell)] = Color::Blue();
	_objectColors[static_cast<int>(ObservedObject::JumpPad)] = Color::Orange();
	_objectColors[static_cast<int>(ObservedObject::Gate)] = Color::Red();
	_objectColors[static_cast<int>(ObservedObject::Teleporter)] = Color::Yellow();
	_objectColors[static_cast<int>(ObservedObject::Elevator)] = Color::Green();
	_objectColors[static_cast<int>(ObservedObject::ConveyorForward)] = Color::Green();
	_objectColors[static_cast<int>(ObservedObject::ConveyorBackward)] = Color::Red();
	_objectColors[static_cast<int>(ObservedObject::Door)] = Color::Green();
}

int ExperienceManager::CreateExperience(const std::vector<CompressedPerceptionGridRectangle>& rectangles, Vector2 velocity)
{
	_createLock.Lock();

	// We write the end perception grid data to the next experience so we know how many compressed
	// rectangles there are, so make sure we have at least 2 experiences left
	if (_nextFreeExperience + 1 >= MaxExperiences)
	{
		ONCE(Log("Out of experiences"));
		return -1;
	}

	auto experience = &_experiences[_nextFreeExperience];
	const auto start = _nextFreePerceptionGridDataIndex;

	for (int i = 0; i < rectangles.size(); ++i)
	{
		if (_nextFreePerceptionGridDataIndex >= PerceptionGridDataSize)
		{
			ONCE(Log("Out of experiences"));
			return -1;
		}

		_compressedPerceptionGridData[_nextFreePerceptionGridDataIndex] = rectangles[i].Data();
		_nextFreePerceptionGridDataIndex++;
	}

	*experience = CompressedExperience(start, velocity);

	int end = _nextFreePerceptionGridDataIndex;
	experience[1].perceptionGridDataStartIndex = end;

	++_nextFreeExperience;

	auto id = experience - _experiences;

	_createLock.Unlock();

	return id;
}

void ExperienceManager::SamplePerceptionGrid(Scene* scene, const Vector2 topLeftPosition, Grid<long long>& outGrid)
{
	const int maxRectangles = 200;
	PerceptionGridRectangle rectangles[maxRectangles];

	outGrid.FillWithZero();

	auto gridRectangles = GetPerceptionGridRectangles(scene, topLeftPosition, rectangles);

	volatile int count = 0;

	for (auto& rectangle : gridRectangles)
	{
		FillGridWithRectangle(outGrid, rectangle);
		++count;
	}
}

void ExperienceManager::SamplePerceptionGrid(
	Scene* scene,
	const Vector2 topLeftPosition,
	std::vector<CompressedPerceptionGridRectangle>& outRectangles)
{
	const int maxRectangles = 200;
	PerceptionGridRectangle rectangles[maxRectangles];

	auto gridRectangles = GetPerceptionGridRectangles(scene, topLeftPosition, rectangles);

	outRectangles.resize(gridRectangles.size());

	for (int i = 0; i < gridRectangles.size(); ++i)
	{
		auto& rect = rectangles[i];

		outRectangles[i] = CompressedPerceptionGridRectangle(
			static_cast<int>(rect.observedObject),
			rect.rectangle.topLeft.x,
			rect.rectangle.topLeft.y,
			rect.rectangle.Width(),
			rect.rectangle.Height());
	}
}

bool IsMultipleOf(float val, float divisor)
{
	return IsApproximately(val - floor(val / divisor) * divisor, 0);
}


gsl::span<PerceptionGridRectangle>  ExperienceManager::GetPerceptionGridRectangles(
	Scene* scene,
	Vector2 topLeftPosition,
	gsl::span<PerceptionGridRectangle> storage)
{
	const int maxOverlapColliders = 100;
	ColliderHandle overlapColliders[maxOverlapColliders];

	const auto gridDimensions = Vector2(TotalColumnWidth(), TotalColumnHeight());


	topLeftPosition = Vector2(
		round(topLeftPosition.x / PerceptionGridCellSize), 
		round(topLeftPosition.y / PerceptionGridCellSize)) * PerceptionGridCellSize;
	const Rectangle bounds(topLeftPosition, gridDimensions);

	auto colliders = scene->FindOverlappingColliders(bounds, overlapColliders);

	const auto perceptionGridBounds = Rectanglei(Vector2i(0, 0), Vector2i(PerceptionGridCols, PerceptionGridRows));

	int total = Min(colliders.size(), storage.size());

	int resultCount = 0;

	for (int i = 0; i < total; ++i)
	{
		auto& collider = colliders[i];
		auto observedObject = collider.OwningEntity()->observedObjectType;

		if (observedObject == static_cast<int>(ObservedObject::Nothing))
		{
			continue;
		}

		auto colliderBounds = collider.Bounds();

		auto topLeftFloor = colliderBounds.TopLeft().Floor().AsVectorOfType<float>();
		auto topLeftPositionFloor = topLeftPosition.Floor().AsVectorOfType<float>();

		auto colliderTopLeft = ((topLeftFloor - topLeftPositionFloor) / PerceptionGridCellSize)
			.Floor()
			.Clamp({ 0, 0 }, { PerceptionGridCols, PerceptionGridRows});

		auto bottomRightFloor = colliderBounds.BottomRight().Floor().AsVectorOfType<float>();

		auto relativeBottomRight = (bottomRightFloor - topLeftPositionFloor) / PerceptionGridCellSize;
		auto colliderBottomRight = relativeBottomRight.Floor();

		colliderBottomRight = colliderBottomRight
			.Clamp(Vector2i(0, 0), Vector2i(PerceptionGridCols, PerceptionGridRows));

		auto colliderRectangle = Rectanglei(colliderTopLeft, colliderBottomRight - colliderTopLeft);
		auto intersection = perceptionGridBounds.Intersect(colliderRectangle);

		storage[resultCount++] = PerceptionGridRectangle(
			Rectanglei(intersection.topLeft - perceptionGridBounds.topLeft, intersection.Size()),
			static_cast<ObservedObject>(observedObject));
	}

	return storage.subspan(0, resultCount);
}

void ExperienceManager::FillColliders(Grid<long long>& outGrid, const CompressedExperience* const experience) const
{
	const int totalDynamicRectangles = TotalPerceptionGridRectangles(experience);

	for (int i = 0; i < totalDynamicRectangles; ++i)
	{
		int rectangleId = experience->perceptionGridDataStartIndex + i;
		const auto compressedRect = GetCompressedGridRectangle(rectangleId);
		const auto decompressedRect = DecompressRectangle(compressedRect);

		FillGridWithRectangle(outGrid, decompressedRect);
	}
}

void ExperienceManager::DecompressExperience(int experienceId, DecompressedExperience& outExperience) const
{
	const auto experience = GetExperience(experienceId);
	DecompressPerceptionGrid(experience, outExperience.perceptionGrid);
	outExperience.velocity = experience->velocity;
}

void ExperienceManager::DecompressPerceptionGrid(const CompressedExperience* experience, Grid<long long>& outGrid) const
{
	outGrid.FillWithZero();
	FillColliders(outGrid, experience);
}

void ExperienceManager::RenderExperience(int experienceId, Renderer* renderer, Vector2 topLeft, float scale)
{
	FixedSizeGrid<long long, PerceptionGridRows, PerceptionGridCols> data;
	const auto experience = GetExperience(experienceId);
	DecompressPerceptionGrid(experience, data);
	RenderPerceptionGrid(data, topLeft, renderer, scale);
}

void ExperienceManager::RenderPerceptionGrid(Grid<long long>& grid, const Vector2 topLeftPosition, Renderer* renderer, float scale)
{
	Vector2 position = topLeftPosition;

	for (int i = 0; i < grid.Rows(); ++i)
	{
		position.x = topLeftPosition.x;

		for (int j = 0; j < grid.Cols(); ++j)
		{
			auto obj = grid[i][j];
			auto color = _objectColors[obj];
			auto rect = Rectangle(position, Vector2(PerceptionGridCellSize, PerceptionGridCellSize) * scale);

			if (!(obj < 0 || obj >= (int)(ObservedObject::TotalObjects)))
			{

				auto color = _objectColors[obj];
				auto rect = Rectangle(position, Vector2(PerceptionGridCellSize, PerceptionGridCellSize) * scale);

				renderer->RenderRectangle(rect, color, -0.1f);
			}

			position.x += PerceptionGridCellSize * scale;
		}

		position.y += PerceptionGridCellSize * scale;
	}

#if true
	float x = topLeftPosition.x;
	for (int i = 0; i < grid.Cols(); ++i)
	{
		Vector2 top(x, topLeftPosition.y);
		Vector2 bottom(x, topLeftPosition.y + TotalColumnHeight());

		// true enlightenment found here - part 1 of 2
		//renderer->RenderLine(top, bottom, Color::Red(), -1);

		x += PerceptionGridCellSize;
	}

	float y = topLeftPosition.y;
	for (int i = 0; i < grid.Rows(); ++i)
	{
		Vector2 left(topLeftPosition.x, y);
		Vector2 right(topLeftPosition.x + TotalColumnWidth(), y);

		// true enlightenment found here - part 2 of 2
		//renderer->RenderLine(left, right, Color(0, 255, 0), -1);

		y += PerceptionGridCellSize;
	}
#endif
}

void ExperienceManager::DiscardExperiencesBefore(int experienceId)
{
	_createLock.Lock();

	if (experienceId == 0)
	{
		_nextFreeExperience = 0;
		_nextFreePerceptionGridDataIndex = 0;
	}
	else
	{
		_nextFreeExperience = experienceId;
		_nextFreePerceptionGridDataIndex = _experiences[experienceId].perceptionGridDataStartIndex;
	}

	_createLock.Unlock();
}

void ExperienceManager::Serialize(BinaryStreamWriter& writer) const
{
	writer.WriteInt(_nextFreeExperience);
	writer.WriteBlob(_experiences, _nextFreeExperience * sizeof(CompressedExperience));

	writer.WriteInt(_nextFreePerceptionGridDataIndex);
	writer.WriteBlob(_compressedPerceptionGridData, _nextFreePerceptionGridDataIndex * sizeof(CompressedPerceptionGridRectangle));
}

int ExperienceManager::Deserialize(BinaryStreamReader& reader)
{
	// We load the data by treating all the positions in the file as offsets so that we can build an
	// experience manager by deserializing multiple files
	int experienceStart = _nextFreeExperience;
	int totalExperiences = reader.ReadInt();

	_nextFreeExperience += totalExperiences;

	printf("%d total experiences in file\n", totalExperiences);

	reader.ReadBlob(_experiences + experienceStart, totalExperiences * sizeof(CompressedExperience));

	int dataSize = reader.ReadInt();
	int dataStart = _nextFreePerceptionGridDataIndex;

	_nextFreePerceptionGridDataIndex += dataSize;

	reader.ReadBlob(_compressedPerceptionGridData + dataStart, dataSize * sizeof(CompressedPerceptionGridRectangle));

	for(int i = experienceStart; i < experienceStart + totalExperiences; ++i)
	{
		GetExperience(i)->perceptionGridDataStartIndex += dataStart;
	}

	return experienceStart;
}

int ExperienceManager::TotalPerceptionGridRectangles(const CompressedExperience* experience)
{
	return experience[1].perceptionGridDataStartIndex - experience[0].perceptionGridDataStartIndex;
}

PerceptionGridRectangle ExperienceManager::DecompressRectangle(const CompressedPerceptionGridRectangle& compressedRect)
{
	return PerceptionGridRectangle(
		Rectanglei(
			Vector2i(compressedRect.X(), compressedRect.Y()),
			Vector2i(compressedRect.Width(), compressedRect.Height())),
		static_cast<ObservedObject>(compressedRect.ObservedObject()));
}

void ExperienceManager::FillGridWithRectangle(Grid<long long>& grid, const PerceptionGridRectangle& rect)
{
	// TODO: move these to constructor
	int gridPriority[static_cast<int>(ObservedObject::TotalObjects)];
	const int observedObject = static_cast<int>(rect.observedObject);

	int priority = -1;
	
	// Order these from lowest-priority-first to highest-priority-last
	gridPriority[static_cast<int>(ObservedObject::Nothing)] = ++priority;
	gridPriority[static_cast<int>(ObservedObject::Gate)] = ++priority;
	gridPriority[static_cast<int>(ObservedObject::ConveyorBackward)] = ++priority;
	gridPriority[static_cast<int>(ObservedObject::ConveyorForward)] = ++priority;
	gridPriority[static_cast<int>(ObservedObject::Breakable)] = ++priority;
	gridPriority[static_cast<int>(ObservedObject::JumpPad)] = ++priority;
	gridPriority[static_cast<int>(ObservedObject::GravityWell)] = ++priority;
	gridPriority[static_cast<int>(ObservedObject::Wall)] = ++priority;
	gridPriority[static_cast<int>(ObservedObject::Door)] = ++priority;
	gridPriority[static_cast<int>(ObservedObject::Teleporter)] = ++priority;
	gridPriority[static_cast<int>(ObservedObject::Elevator)] = ++priority;

	const auto observedObjectPriority = gridPriority[observedObject];
	for (int y = rect.rectangle.topLeft.y; y < rect.rectangle.bottomRight.y; ++y)
	{
		for (int x = rect.rectangle.topLeft.x; x < rect.rectangle.bottomRight.x; ++x)
		{
			auto& gridCell = grid[y][x];
			if (observedObjectPriority > gridPriority[gridCell])
			{
				gridCell = observedObject;
			}
		}
	}
}

CompressedPerceptionGridRectangle ExperienceManager::GetCompressedGridRectangle(int rectangleId) const
{
	if (rectangleId < 0 || rectangleId >= _nextFreePerceptionGridDataIndex)
	{
		FatalError("Invalid rectangleId: %d", rectangleId);
	}

	return CompressedPerceptionGridRectangle(_compressedPerceptionGridData[rectangleId]);
}

const CompressedExperience* ExperienceManager::GetExperience(int experienceId) const
{
	if (experienceId >= _nextFreeExperience || experienceId < 0)
	{
		FatalError("Invalid experience id: %d (max is %d)", experienceId, _nextFreeExperience);
	}

	return &_experiences[experienceId];
}

CompressedExperience* ExperienceManager::GetExperience(int experienceId)
{
	return const_cast<CompressedExperience*>(const_cast<const ExperienceManager*>(this)->GetExperience(experienceId));
}

void SampleManager::Reset()
{
	for (int i = 0; i < (int)CharacterAction::TotalActions; ++i)
	{
		auto action = (CharacterAction)i;
		experiencesByActionType[action].Clear();
	}

	experienceManager.DiscardExperiencesBefore(0);
}

void SampleManager::Serialize(BinaryStreamWriter& writer)
{
	experienceManager.Serialize(writer);

	writer.WriteInt((int)CharacterAction::TotalActions);

	for (int i = 0; i < (int)CharacterAction::TotalActions; ++i)
	{
		auto& experiences = experiencesByActionType[(CharacterAction)i];
		writer.WriteInt(experiences.Size());
		writer.WriteBlob(&experiences[0], sizeof(int) * experiences.Size());
	}

}

void SampleManager::Deserialize(BinaryStreamReader& reader)
{
	int firstExperience = experienceManager.Deserialize(reader);
	Assert(reader.ReadInt() == (int)CharacterAction::TotalActions, "Wrong number of player actions in data file");

	for (int i = 0; i < (int)CharacterAction::TotalActions; ++i)
	{
		auto& actions = experiencesByActionType[(CharacterAction)i];

		int totalNewActions = reader.ReadInt();
		int firstAction = actions.Size();

		actions.Resize(actions.Size() + totalNewActions);
		reader.ReadBlob(&actions[firstAction], sizeof(int) * actions.Size());

		// Treat the experience numbers as offsets from the first experience
		for(int j = firstAction; j < firstAction + totalNewActions; ++j)
		{
			actions[j] += firstExperience;
		}
	}
}

bool SampleManager::TryGetRandomSample(CharacterAction type, gsl::span<DecompressedExperience> outExperiences)
{
	auto& actionExperiences = experiencesByActionType[type];

	auto lowestIndex = 0;
	while (lowestIndex < actionExperiences.Size())
	{
		std::uniform_int_distribution<int> distribution(lowestIndex, actionExperiences.Size() - 1);

		int experienceIndex = distribution(generator);
		int experienceId = actionExperiences[experienceIndex];

		if (experienceId < outExperiences.size())
		{
			lowestIndex = experienceIndex + 1;
		}
		else
		{
			for (int i = 0; i < outExperiences.size(); ++i)
			{
				experienceManager.DecompressExperience(experienceId - i, outExperiences[outExperiences.size() - 1 - i]);
			}

			return true;
		}
	}

	return false;
}

void SampleManager::AddSample(int experienceId, CharacterAction action)
{
	experiencesByActionType[action].PushBack(experienceId);
}

bool SampleManager::HasSamples(CharacterAction actionType)
{
	return experiencesByActionType[actionType].Size() != 0;
}

bool SampleManager::HasSamples()
{
	for (int i = 0; i < (int)CharacterAction::TotalActions; ++i)
	{
		if (HasSamples((CharacterAction)i))
		{
			return true;
		}
	}

	return false;
}


