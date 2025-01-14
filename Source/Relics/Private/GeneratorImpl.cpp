#include "GeneratorImpl.h"

bool GeneratorImpl::generate()
{
	round();
	return placeStuff();
}

const std::vector<RoomImpl>& GeneratorImpl::getRooms() const
{
	return rooms;
}

RandomGenerator& GeneratorImpl::getRandomGenerator()
{
	return rg;
}

void GeneratorImpl::round()
{
	const auto max = static_cast<float>(size);
	const auto center = max / 2.f;
	for (float i = 0; i < max; i++)
	{
		for (float j = 0; j < max; j++)
		{
			auto d = static_cast<float>(sqrt(
				pow(center - (i > center ? i : i + 1), 2) + center
				+ pow(center - (j > center ? j : j + 1), 2) + center));
			if (d > center)
			{
				grid.set(i, j, 'X');
			}
		}
	}
}

bool GeneratorImpl::placeStuff()
{
	int id = 0;
	int retries = 0;
	while (openSpace())
	{
		unless(placeThing(id))
		{
			if (++retries > 5)
			{
				return false;
			}
		}
		else
		{
			retries = 0;
			if (++id + '0' == 'X')
			{
				id++;
			};

			std::cout << *this << std::endl;
		}
	}
	return true;
}

bool GeneratorImpl::openSpace() const
{
	for (auto i = 0; i < size; i++)
	{
		for (auto j = 0; j < size; j++)
		{
			int s = 0;
			while (grid.isEmpty(i, j, s, gap))
			{
				s++;
			}
			if (s > room_min)
			{
				return true;
			}
		}
	}
	return false;
}

bool GeneratorImpl::placeThing(const char id)
{
	const int width = rg.getRandom(room_min, room_max);
	const int height = rg.getRandom(room_min, room_max);

	if (width >= 3 && height >= 3)
	{
		for (auto i = 0; i < size - height; i++)
		{
			for (auto j = 0; j < size - width; j++)
			{
				if (grid.isEmpty(i, j, width, height, gap))
				{
					rooms.emplace_back(id, i, j, width, height, rg);
					rooms[rooms.size() - 1].draw(grid);
					return true;
				}
			}
		}
	}
	return false;
}

GeneratorImpl::GeneratorImpl(const int size, const int room_min, const int room_max,
                             const int gap, const int seed) :
	grid(TwoDArray(size, size)), size(size), room_min(room_min),
	room_max(room_max), gap(gap), rg(RandomGenerator(seed))
{
}

GeneratorImpl::~GeneratorImpl() = default;
