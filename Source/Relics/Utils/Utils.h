#pragma once

#include <random>
#include <limits>

#define unless(cond) if (!(cond))

class RandomGenerator {
	std::mt19937 gen; // Mersenne Twister engine
	unsigned int seed;
public:
	// Constructor with an optional seed parameter
	explicit RandomGenerator(unsigned int seed = std::random_device{}())
		: gen(seed), seed(seed) {}

	// Method to generate a random number
	inline int getRandom(const int min = 0, const int max = std::numeric_limits<int>::max()) {
		std::uniform_int_distribution<> distrib(min, max);
		return distrib(gen);
	}

	[[nodiscard]] unsigned int getSeed() const
	{
		return seed;
	}
};