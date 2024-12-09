#include "Utils.h"

#include <random>

int getRandomInt()
{
	std::random_device rd;

	// Use Mersenne Twister engine for random number generation
	std::mt19937 gen(rd());

	std::uniform_int_distribution<> distrib(0, std::numeric_limits<int>::max());

	return distrib(gen);
}

int random_in_range(int min, int max)
{
	return std::rand() % (max - min + 1) + min;
}