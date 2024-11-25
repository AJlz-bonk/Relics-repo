#include "Relics/Public/DungeonGenerator.h"
#include <cstdlib>
#include <ctime>
#include <map>

enum
{
	NOTHING = 0x00000000,
	BLOCKED = 0x00000001,
	ROOM = 0x00000002,
	CORRIDOR = 0x00000004,
	PERIMETER = 0x00000010,
	ENTRANCE = 0x00000020,
	ROOM_ID = 0x0000FFC0,

	ARCH = 0x00010000,
	DOOR = 0x00020000,
	LOCKED = 0x00040000,
	TRAPPED = 0x00080000,
	SECRET = 0x00100000,
	PORTC = 0x00200000,
	STAIR_DN = 0x00400000,
	STAIR_UP = 0x00800000,

	LABEL = 0xFF000000,

	OPENSPACE = ROOM | CORRIDOR,
	DOORSPACE = ARCH | DOOR | LOCKED | TRAPPED | SECRET | PORTC,
	ESPACE = ENTRANCE | DOORSPACE | 0xFF000000,
	STAIRS = STAIR_DN | STAIR_UP,

	BLOCK_ROOM = BLOCKED | ROOM,
	BLOCK_CORR = BLOCKED | PERIMETER | CORRIDOR,
	BLOCK_DOOR = BLOCKED | DOORSPACE,
};

class TwoDArray
{
	int* data;
	int rows, cols;

public:
	TwoDArray(int r, int c) : rows(r), cols(c)
	{
		data = new int[rows * cols];
	}

	~TwoDArray()
	{
		delete[] data;
	}

	int& get(int i, int j)
	{
		return data[i * cols + j];
	}

	void set(int i, int j, int val)
	{
		data[i * cols + j] = val;
	}
};

class DungeonRoom
{
public:
	DungeonRoom(int id, int row, int col, int north, int south, int west, int east, int height, int width, int area);
	int id;
	int row;
	int col;
	int north;
	int south;
	int west;
	int east;
	int height;
	int width;
	int area;
};

DungeonRoom::DungeonRoom(int id, int row, int col, int north, int south, int west, int east, int height, int width,
                         int area)
	: id(id), row(row), col(col), north(north), south(south), west(west), east(east), height(height), width(width),
	  area(area)
{
}

class Dungeon
{
	int n_rows;
	int n_cols;
	int n_i;
	int n_j;
	int max_row;
	int max_col;
	int n_rooms;
	int room_base;
	int room_radix;
	int room_min;
	int room_max;
	int seed;
	std::map<int, DungeonRoom*> room;
	TwoDArray* cell;
	void round_mask();
	void pack_rooms();
	void emplace_room(int i, int j);
	void set_room(int& i, int& j, int& width, int& height);
	void sound_room(int r1, int c1, int r2, int c2, bool& blocked, std::map<int, int>& hit);

public:
	Dungeon(int n_rows, int n_cols, int room_min, int room_max, int seed);
	~Dungeon();
	void init_cells();
	void emplace_rooms();
};

Dungeon::Dungeon(int n_rows, int n_cols, int room_min, int room_max, int seed)
	: n_rows(n_rows / 2 * 2), n_cols(n_cols / 2 * 2), n_rooms(0), room_min(room_min), room_max(room_max), seed(seed)
{
	this->n_i = this->n_rows / 2;
	this->n_j = this->n_cols / 2;
	this->max_row = this->n_rows - 1;
	this->max_col = this->n_cols - 1;
	this->room_base = (this->room_min + 1) / 2;
	this->room_radix = (this->room_max - this->room_min) / 2 + 1;
	this->cell = new TwoDArray(this->n_rows, this->n_cols);
	this->room = std::map<int, DungeonRoom*>();
}

Dungeon::~Dungeon()
{
	delete cell;
	for(std::map<int, DungeonRoom*>::iterator i = room.begin(); i != room.end(); i=room.erase(i))
	{
		delete i->second;
	}
}

void Dungeon::init_cells()
{
	for (int i = 0; i < n_rows; i++)
	{
		for (int j = 0; j < n_cols; j++)
		{
			cell->set(i, j, NOTHING);
		}
	}
	std::srand(seed);
	round_mask();
}

void Dungeon::round_mask()
{
	int center_r = n_rows / 2;
	int center_c = n_cols / 2;

	for (int i = 0; i < n_rows; i++)
	{
		for (int j = 0; j < n_cols; j++)
		{
			double d = sqrt((pow(i - center_r, 2) + pow(j - center_c, 2)));
			if (d > center_c)
			{
				cell->set(i, j, BLOCKED);
			}
		}
	}
}

void Dungeon::emplace_rooms()
{
	pack_rooms();
}

void Dungeon::pack_rooms()
{
	for (int i = 0; i < n_i; i++)
	{
		int r = i * 2 + 1;
		for (int j = 0; j < n_j; j++)
		{
			int c = j * 2 + 1;
			if ((cell->get(i, j) & ROOM) || ((i == 0 || j == 0) && int(std::rand() * 2)))
			{
				continue;
			}
			emplace_room(i, j);
		}
	}
}

void Dungeon::emplace_room(int i, int j)
{
	if (n_rooms == 999)
	{
		return;
	}
	int width = 0;
	int height = 0;
	set_room(i, j, width, height);

	int r1 = i * 2 + 1;
	int c1 = j * 2 + 1;
	int r2 = (i + height) * 2 - 1;
	int c2 = (j + width) * 2 - 1;

	if ((r1 < 1 || r2 > max_row) || (c1 < 1 || c2 > max_col))
	{
		return;
	}

	bool blocked;
	std::map<int, int> hit = std::map<int, int>();
	sound_room(r1, c1, r2, c2, blocked, hit);
	if (blocked)
	{
		return;
	}
	if (hit.size() > 0)
	{
		return;
	}
	int room_id = ++n_rooms;

	for (int r = r1; r <= r2; r++)
	{
		for (int c = c1; c <= c2; c++)
		{
			if (cell->get(r, c) & ENTRANCE)
			{
				cell->set(r, c, cell->get(r, c) & ~ESPACE);
			}
			else if (cell->get(r, c) & PERIMETER)
			{
				cell->set(r, c, cell->get(r, c) & ~PERIMETER);
			}
			cell->set(r, c, cell->get(r, c) | ROOM | (room_id << 6));
		}
	}
	int h = (r2 - r1 + 1) * 10;
	int w = (c2 - c1 + 1) * 10;
	
	room[room_id] = new DungeonRoom(room_id, r1, c1, r1, r2, c1, c2, h, w, h*w);

	for (int r = r1 - 1; r <= r2 + 1; r++)
	{
		if(!(cell->get(r,c1-1)&(ROOM | ENTRANCE)))
		{
			cell->set(r,c1-1,cell->get(r,c1-1) | PERIMETER);
		}
		if(!(cell->get(r,c2+1)&(ROOM | ENTRANCE)))
		{
			cell->set(r,c2+1,cell->get(r,c1-1) | PERIMETER);
		}
	}
	for (int c = c1 - 1; c <= c2 + 1; c++)
	{
		if(!(cell->get(r1 - 1, c) & (ROOM | ENTRANCE)))
		{
			cell->set(r1 - 1, c, cell->get(r1 - 1, c) | PERIMETER);
		}
		if (!(cell->get(r2 + 1, c)& (ROOM | ENTRANCE)))
		{
			cell->set(r2 + 1, c, cell->get(r2 + 1, c) | PERIMETER);
		}
	}
}

void Dungeon::set_room(int& i, int& j, int& width, int& height)
{
	if (height == 0)
	{
		if (i)
		{
			height = int(std::rand() * std::min(std::max(n_i - room_base - i, 0), room_radix)) + room_base;
		}
		else
		{
			height = int(std::rand() * room_radix) + room_base;
		}
	}
	if (width == 0)
	{
		if (j)
		{
			width = int(std::rand() * std::min(std::max(n_j - room_base - j, 0), room_radix)) + room_base;
		}
		else
		{
			width = int(std::rand() * room_radix) + room_base;
		}
	}
	if (i == 0)
	{
		i = int(std::rand() * (n_i - height));
	}
	if (j == 0)
	{
		j = int(std::rand() * (n_j - width));
	}
}

void Dungeon::sound_room(int r1, int c1, int r2, int c2, bool& blocked, std::map<int, int>& hit)
{
	blocked = false;
	for (int r = r1; r <= r2; r++)
	{
		for (int c = c1; c <= c2; c++)
		{
			if (cell->get(r, c) & BLOCKED)
			{
				blocked = true;
				return;
			}
			if (cell->get(r, c) & ROOM)
			{
				hit[(cell->get(r, c) & ROOM_ID) >> 6]++;
			}
		}
	}
}


DungeonGenerator::DungeonGenerator()
{
	Dungeon* dungeon = new Dungeon(39, 39, 3, 9, std::time(nullptr));
	dungeon->init_cells();
	dungeon->emplace_rooms();
}

DungeonGenerator::~DungeonGenerator()
{
}