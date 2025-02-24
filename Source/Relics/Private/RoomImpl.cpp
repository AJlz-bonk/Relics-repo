#include "RoomImpl.h"

#include <algorithm>

void RoomImpl::drawLine(const bool isVert, std::pair<int, int>* wall, std::pair<int, int>* next, TwoDArray& grid) const
{
	//char ch = (id % (126 - 33)) + 33;
	char ch = id % (126 - 48) + 48;

	if (isVert)
	{
		int start = std::min(wall->first, next->first);
		int end = std::max(wall->first, next->first);
		for (int i = start; i <= end; i++)
		{
			if (!doors.contains({i, wall->second}))
			{
				grid.set(i + row, wall->second + col, ch);
			}
			else
			{
				grid.set(i + row, wall->second + col, ' ');
			}
		}
	}
	else
	{
		int start = std::min(wall->second, next->second);
		int end = std::max(wall->second, next->second);
		for (int j = start; j <= end; j++)
		{
			if (!doors.contains({wall->first, j}))
			{
				grid.set(wall->first + row, j + col, ch);
			}
			else
			{
				grid.set(wall->first + row, j + col, ' ');
			}
		}
	}
}

void RoomImpl::reshapeL(RandomGenerator& rg)
{
	int min_size = 3;
	if (static_cast<int>(width) - 1 <= min_size + min_size || static_cast<int>(height) - 1 <= min_size + min_size)
	{
		return;
	}
	unsigned int wall = rg.getRandom(0, static_cast<int>(walls.size()) - 1);
	unsigned int c = rg.getRandom(min_size, static_cast<int>(width - 1) - min_size);
	unsigned int r = rg.getRandom(min_size, static_cast<int>(height - 1) - min_size);

	std::vector<std::pair<int, int>> newWalls;
	if (wall == 0)
	{
		newWalls.emplace_back(walls[0]);
		newWalls.emplace_back(r, walls[0].second);
		newWalls.emplace_back(r, c);
		newWalls.emplace_back(walls[1].first, c);
		newWalls.emplace_back(walls[2]);
		newWalls.emplace_back(walls[3]);
	}
	else if (wall == 1)
	{
		newWalls.emplace_back(walls[0]);
		newWalls.emplace_back(walls[1]);
		newWalls.emplace_back(walls[1].first, c);
		newWalls.emplace_back(r, c);
		newWalls.emplace_back(r, walls[2].second);
		newWalls.emplace_back(walls[3]);
	}
	else if (wall == 2)
	{
		newWalls.emplace_back(walls[0]);
		newWalls.emplace_back(walls[1]);
		newWalls.emplace_back(walls[2]);
		newWalls.emplace_back(r, walls[2].second);
		newWalls.emplace_back(r, c);
		newWalls.emplace_back(walls[3].first, c);
	}
	else if (wall == 3)
	{
		newWalls.emplace_back(walls[3].first, c);
		newWalls.emplace_back(r, c);
		newWalls.emplace_back(r, walls[0].second);
		newWalls.emplace_back(walls[1]);
		newWalls.emplace_back(walls[2]);
		newWalls.emplace_back(walls[3]);
	}
	walls = newWalls;
}

void RoomImpl::reshapeU(RandomGenerator& rg)
{
	//minimum interior width of cuts
	//4 means 3 empties between walls
	int min_interior = 3 + 1;
	//minimum size of interior
	//4 means 3 empties between walls
	int min_size = 3 + 1;
	unsigned int wall = rg.getRandom(0, static_cast<int>(walls.size()) - 1);

	//if minimum interior width is too big for the room, decrease it until a cut will fit
	//if the room is so small that the points are forced to generate next to each other, do not create a cut
	while (min_interior > static_cast<int>(std::min(height, width)) - 1 - min_size - min_size)
	{
		if (--min_interior <= 2)
		{
			return;
		}
	}

	std::vector<std::pair<int, int>> newWalls;
	if (wall == 0)
	{
		unsigned int c = rg.getRandom(0, static_cast<int>(width) - 1 - min_size - min_size) + min_size;
		unsigned int r1 = rg.getRandom(0, static_cast<int>(height) - 1 - min_size - min_size - min_interior);
		unsigned int r2 = rg.getRandom(static_cast<int>(r1) + min_interior,static_cast<int>(height) - 1 - min_size - min_size);
		r1 += min_size;
		r2 += min_size;

		newWalls.emplace_back(walls[0]);
		newWalls.emplace_back(r1, walls[0].second);
		newWalls.emplace_back(r1, c);
		newWalls.emplace_back(r2, c);
		newWalls.emplace_back(r2, walls[0].second);
		newWalls.emplace_back(walls[1]);
		newWalls.emplace_back(walls[2]);
		newWalls.emplace_back(walls[3]);
	}
	else if (wall == 1)
	{
		unsigned int r = rg.getRandom(0, static_cast<int>(height) - 1 - min_size - min_size) + min_size;
		unsigned int c1 = rg.getRandom(0, static_cast<int>(width) - 1 - min_size - min_size - min_interior);
		unsigned int c2 = rg.getRandom(static_cast<int>(c1) + min_interior,static_cast<int>(width) - 1 - min_size - min_size);
		c1 += min_size;
		c2 += min_size;

		newWalls.emplace_back(walls[0]);
		newWalls.emplace_back(walls[1]);
		newWalls.emplace_back(walls[1].first, c1);
		newWalls.emplace_back(r, c1);
		newWalls.emplace_back(r, c2);
		newWalls.emplace_back(walls[1].first, c2);
		newWalls.emplace_back(walls[2]);
		newWalls.emplace_back(walls[3]);
	}
	else if (wall == 2)
	{
		unsigned int c = rg.getRandom(0, static_cast<int>(width) - 1 - min_size - min_size) + min_size;
		unsigned int r1 = rg.getRandom(0, static_cast<int>(height) - 1 - min_size - min_size - min_interior);
		unsigned int r2 = rg.getRandom(static_cast<int>(r1) + min_interior,static_cast<int>(height) - 1 - min_size - min_size);
		r1 += min_size;
		r2 += min_size;

		newWalls.emplace_back(walls[0]);
		newWalls.emplace_back(walls[1]);
		newWalls.emplace_back(walls[2]);
		newWalls.emplace_back(r2, walls[2].second);
		newWalls.emplace_back(r2, c);
		newWalls.emplace_back(r1, c);
		newWalls.emplace_back(r1, walls[2].second);
		newWalls.emplace_back(walls[3]);
	}
	else if (wall == 3)
	{
		unsigned int r = rg.getRandom(0, static_cast<int>(height) - 1 - min_size - min_size) + min_size;
		unsigned int c1 = rg.getRandom(0, static_cast<int>(width) - 1 - min_size - min_size - min_interior);
		unsigned int c2 = rg.getRandom(static_cast<int>(c1) + min_interior,static_cast<int>(width) - 1 - min_size - min_size);
		c1 += min_size;
		c2 += min_size;

		newWalls.emplace_back(walls[0]);
		newWalls.emplace_back(walls[1]);
		newWalls.emplace_back(walls[2]);
		newWalls.emplace_back(walls[3]);
		newWalls.emplace_back(walls[3].first, c2);
		newWalls.emplace_back(r, c2);
		newWalls.emplace_back(r, c1);
		newWalls.emplace_back(walls[3].first, c1);
	}
	walls = newWalls;
}

void RoomImpl::reshapeO(RandomGenerator& rg)
{
	//minimum interior width of courtyard
	//4 means 3 empties between walls
	int min_interior = 3 + 1;
	//minimum size of interior
	//4 means 3 empties between walls
	int min_size = 3 + 1;

	//if minimum interior width is too big for the room, decrease it until a hole will fit
	//if the room is so small that there is only one valid interior location, do not create a hole
	while (min_interior > static_cast<int>(std::min(width, height)) - 1 - min_size - min_size)
	{
		if (--min_interior <= 1)
		{
			return;
		}
	}

	std::vector<std::pair<int, int>> newWalls;

	unsigned int c1 = rg.getRandom(0, static_cast<int>(width) - 1 - min_size - min_size - min_interior);
	unsigned int c2 = rg.getRandom(static_cast<int>(c1) + min_interior,
	                               static_cast<int>(width) - 1 - min_size - min_size);
	unsigned int r1 = rg.getRandom(0, static_cast<int>(height) - 1 - min_size - min_size - min_interior);
	unsigned int r2 = rg.getRandom(static_cast<int>(r1) + min_interior,
	                               static_cast<int>(height) - 1 - min_size - min_size);
	r1 += min_size;
	r2 += min_size;
	c1 += min_size;
	c2 += min_size;

	newWalls.emplace_back(r1, c1);
	newWalls.emplace_back(r2, c1);
	newWalls.emplace_back(r2, c2);
	newWalls.emplace_back(r1, c2);

	interior_walls = newWalls;
}

void RoomImpl::addDoors(RandomGenerator& rg)
{
	bool isVert = true;
	bool hasDoor = false;
	std::pair<int, int>* wall = nullptr;
	for (auto& next : walls)
	{
		if (wall)
		{
			addDoorToWall(rg, isVert, next, wall, hasDoor);
			isVert = !isVert;
			hasDoor = false;
		}
		wall = &next;
	}
	addDoorToWall(rg, isVert, walls[0], &walls[walls.size() - 1], hasDoor);
	isVert = !isVert;
	hasDoor = false;

	unless(interior_walls.empty())
	{
		for (auto& next : interior_walls)
		{
			if (wall)
			{
				addDoorToWall(rg, isVert, next, wall, hasDoor);
				isVert = !isVert;
				hasDoor = false;
			}
			wall = &next;
		}
	}
	addDoorToWall(rg, isVert, walls[0], &walls[walls.size() - 1], hasDoor);
}

void RoomImpl::addDoorToWall(RandomGenerator& rg, bool isVert, std::pair<int, int> next, std::pair<int, int>* wall,
                             bool hasDoor)
{
	int threshold = 4;
	int wall_len = (isVert ? std::abs(next.first - wall->first) : std::abs(next.second - wall->second)) - 1;

	if (wall_len > 0)
	{
		for (int i = 0; i <= wall_len / threshold; i++)
		{
			if (rg.getRandom(0, 1) > 0 || (i == wall_len / threshold && !hasDoor))
			{
				int door_y = isVert ? next.second : std::min(next.second, wall->second) + rg.getRandom(1, wall_len);
				int door_x = isVert ? std::min(next.first, wall->first) + rg.getRandom(1, wall_len) : next.first;

				doors.insert({door_x, door_y});
				hasDoor = true;
			}
		}
	}
}

void RoomImpl::draw(TwoDArray& grid)
{
	bool isVert = true;
	std::pair<int, int>* wall = nullptr;
	for (auto& next : walls)
	{
		if (wall)
		{
			drawLine(isVert, wall, &next, grid);
			isVert = !isVert;
		}
		wall = &next;
	}
	drawLine(isVert, wall, &walls[0], grid);

	unless(interior_walls.empty())
	{
		for (auto& next : interior_walls)
		{
			if (wall)
			{
				drawLine(isVert, wall, &next, grid);
				isVert = !isVert;
			}
			wall = &next;
		}
		drawLine(isVert, wall, &interior_walls[0], grid);
	}
}

unsigned int RoomImpl::getId() const
{
	return id;
}

unsigned int RoomImpl::getRow() const
{
	return row;
}

unsigned int RoomImpl::getCol() const
{
	return col;
}

unsigned int RoomImpl::getWidth() const
{
	return width;
}

unsigned int RoomImpl::getHeight() const
{
	return height;
}

std::set<std::pair<int, int>>& RoomImpl::getDoors()
{
	return doors;
}

std::vector<std::pair<int, int>>& RoomImpl::getWalls()
{
	return walls;
}

std::vector<std::pair<int, int>>& RoomImpl::getInteriorWalls()
{
	return interior_walls;
}

RoomImpl::RoomImpl(int id, int row, int col, int width, int height, RandomGenerator& rg) :
	id(id), row(row), col(col), width(width), height(height),
	walls({{0, 0}, {height - 1, 0}, {height - 1, width - 1}, {0, width - 1}})
{
	if (width >= 3 && height >= 3)
	{
		int chance = rg.getRandom(0, 100);
		if (chance > 75)
		{
			reshapeL(rg);
		}
		else if (chance > 50)
		{
			reshapeU(rg);
		}
		else if (chance > 35)
		{
			reshapeO(rg);
		}

		addDoors(rg);
	}
}

RoomImpl::RoomImpl()
	: id(0), row(0), col(0), width(0), height(0)
{
}
