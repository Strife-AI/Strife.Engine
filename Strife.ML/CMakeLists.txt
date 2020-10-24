cmake_minimum_required(VERSION 3.2 FATAL_ERROR)

add_library(Strife.ML STATIC
	ExperienceManager.cpp
	Decider.cpp
	Trainer.cpp
	LearningRateSchedule.cpp
	NeuralNetwork.cpp
)

find_package(torch REQUIRED)
target_link_libraries(Strife.ML PUBLIC torch Strife.Engine)

set_property(TARGET Strife.ML PROPERTY CXX_STANDARD 17)