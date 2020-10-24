#include "LearningRateSchedule.hpp"
#include "Tools/ConsoleVar.hpp"

#ifndef RELEASE_BUILD
ConsoleVar<float> g_learningRate("lr", 1e-5f, true);
#else
ConsoleVar<float> g_learningRate("lr", 1e-5f, false);
#endif

float ConstantLearningRateSchedule::GetCurrentLearningRate()
{
	return g_learningRate.Value();
}

SuperConvergenceLearningRateSchedule::SuperConvergenceLearningRateSchedule(float minRate, float maxRate, int cycleLength)
    : _minRate(minRate),
    _maxRate(maxRate),
	_cycleLength(cycleLength)
{
}

float SuperConvergenceLearningRateSchedule::GetCurrentLearningRate()
{
	_step++;
	auto midPoint = _cycleLength / 2.0f;

	if (_step % _cycleLength > midPoint) // rising edge
	{
		auto lerp = (_step % _cycleLength) / midPoint;
		return _minRate * (1 - lerp) + _maxRate * lerp;
	}
	else // falling edge
	{
		auto lerp = (_step % _cycleLength) / midPoint;
		return _maxRate * (1 - lerp) + _minRate * lerp;
	}
}
