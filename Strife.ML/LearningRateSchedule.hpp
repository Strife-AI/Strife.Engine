#pragma once

class ILearningRateSchedule
{
public:
	virtual ~ILearningRateSchedule() = default;

	void Step()
	{
		_step++;
	}
	
	virtual float GetCurrentLearningRate() = 0;
protected:
	int _step = -1;
};

class ConstantLearningRateSchedule : public ILearningRateSchedule
{
public:	
	float GetCurrentLearningRate() override;
};

class SuperConvergenceLearningRateSchedule : public ILearningRateSchedule
{
public:
	SuperConvergenceLearningRateSchedule(float minRate, float maxRate, int cycleLength);
	float GetCurrentLearningRate() override;
private:
	float _minRate;
	float _maxRate;
	int _cycleLength;
};