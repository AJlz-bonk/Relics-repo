#include "Relics/Public/DungeonGenerator.h"
#include <cstdlib>
#include <ctime>
#include <map>
#include <optional>
#include <string>
#include <vector>
#include <cmath>

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

std::map<std::string, int> di = {{"north", -1}, {"south", 1}, {"west", 0}, {"east", 0}};
std::map<std::string, int> dj = {{"north", 0}, {"south", 0}, {"west", -1}, {"east", 1}};
std::map<std::string, std::string> opposite =
{
	{"north", "south"}, {"south", "north"}, {"west", "east"}, {"east", "west"}
};

template <typename T>
class TwoDArray
{
	T* data;
	int cols;

public:
	TwoDArray(int r, int c) : cols(c), data(new T[r * c])
	{
	}

	~TwoDArray()
	{
		delete[] data;
	}

	T& get(int i, int j)
	{
		return data[i * cols + j];
	}

	void set(int i, int j, T val)
	{
		data[i * cols + j] = val;
	}
};

class CloseEnd
{
public:
	std::vector<std::pair<int, int>> walled;
	std::vector<std::pair<int, int>> close;
	std::pair<int, int> recurse;
	CloseEnd(std::vector<std::pair<int, int>> walled, std::vector<std::pair<int, int>> close,
	         std::pair<int, int> recurse);
};

CloseEnd::CloseEnd(std::vector<std::pair<int, int>> walled, std::vector<std::pair<int, int>> close,
                   std::pair<int, int> recurse)
	: walled(walled), close(close), recurse(recurse)
{
}

class Door
{
public:
	int row;
	int col;
	std::string key;
	std::string type;
	int out_id;
	Door(int row, int col, std::string key, std::string type, int out_id);
};

Door::Door(int row, int col, std::string key, std::string type, int out_id)
	: row(row), col(col), key(key), type(type), out_id(out_id)
{
}

class Sill
{
public:
	int sill_r;
	int sill_c;
	std::string dir;
	int door_r;
	int door_c;
	int out_id;
	Sill(int sill_r, int sill_c, std::string dir, int door_r, int door_c, int out_id);
};

Sill::Sill(int sill_r, int sill_c, std::string dir, int door_r, int door_c, int out_id)
	: sill_r(sill_r), sill_c(sill_c), dir(dir), door_r(door_r), door_c(door_c), out_id(out_id)
{
}

int random_in_range(int min, int max)
{
	return std::rand() % (max - min + 1) + min;
}

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
	std::map<std::string, std::vector<Door>> door;
};

DungeonRoom::DungeonRoom(int id, int row, int col, int north, int south, int west, int east, int height, int width,
                         int area)
	: id(id), row(row), col(col), north(north), south(south), west(west), east(east), height(height), width(width),
	  area(area), door(std::map<std::string, std::vector<Door>>())
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
	int deadends_percent;
	std::map<int, DungeonRoom*> rooms;
	std::vector<std::pair<int, int>> connections;
	TwoDArray<int>* cell;
	std::map<std::string, CloseEnd> close_ends;
	std::vector<Door> doors;
	void round_mask();
	void pack_rooms();
	void emplace_room(int i, int j);
	void set_room(int& i, int& j, int& width, int& height);
	void sound_room(int r1, int c1, int r2, int c2, bool& blocked, std::map<int, int>& hit);
	void open_room(DungeonRoom* room);
	std::vector<Sill> door_sills(DungeonRoom* room);
	std::optional<Sill> check_sill(DungeonRoom* room, int sill_r, int sill_c, std::string dir);
	template <typename T>
	std::vector<T> shuffle(std::vector<T> sills);
	int alloc_opens(DungeonRoom* room);
	int door_type();
	void tunnel(int i, int j, std::optional<std::string> last_dir);
	template <typename T>
	std::vector<std::string> keys_of(std::map<std::string, T> the_map);
	bool open_tunnel(int i, int j, std::string dir);
	bool sound_tunnel(int mid_r, int mid_c, int next_r, int next_c);
	void delve_tunnel(int mid_r, int mid_c, int next_r, int next_c);
	void collapse_tunnels(int p, std::map<std::string, CloseEnd> close_end);
	void collapse(int r, int c, std::map<std::string, CloseEnd> xc);
	bool check_tunnel(int r, int c, CloseEnd xc);

public:
	Dungeon(int n_rows, int n_cols, int room_min, int room_max, int seed, int deadends_percent);
	~Dungeon();
	void init_cells();
	void emplace_rooms();
	void open_rooms();
	void label_rooms();
	void corridors();
	void fix_doors();
	void clean_dungeon();
};

Dungeon::Dungeon(int n_rows, int n_cols, int room_min, int room_max, int seed, int deadends_percent)
	: n_rows(n_rows / 2 * 2), n_cols(n_cols / 2 * 2), n_rooms(0), room_min(room_min), room_max(room_max), seed(seed),
	  deadends_percent(deadends_percent)
{
	this->n_i = this->n_rows / 2;
	this->n_j = this->n_cols / 2;
	this->max_row = this->n_rows - 1;
	this->max_col = this->n_cols - 1;
	this->room_base = (this->room_min + 1) / 2;
	this->room_radix = (this->room_max - this->room_min) / 2 + 1;
	this->cell = new TwoDArray<int>(this->n_rows, this->n_cols);
	this->rooms = std::map<int, DungeonRoom*>();
	this->doors = std::vector<Door>();
	this->close_ends = std::map<std::string, CloseEnd>({
		{
			"north",
			CloseEnd(
				std::vector<std::pair<int, int>>({{0, -1}, {1, -1}, {1, 0}, {1, 1}, {0, 1}}),
				std::vector<std::pair<int, int>>({{0, 0}}),
				std::pair<int, int>({-1, 0})
			)
		},
		{
			"south",
			CloseEnd(
				std::vector<std::pair<int, int>>({{0, -1}, {-1, -1}, {-1, 0}, {-1, 1}, {0, 1}}),
				std::vector<std::pair<int, int>>({{0, 0}}),
				std::pair<int, int>({1, 0})
			)
		},
		{
			"west",
			CloseEnd(
				std::vector<std::pair<int, int>>({{-1, 0}, {-1, 1}, {0, 1}, {1, 1}, {1, 0}}),
				std::vector<std::pair<int, int>>({{0, 0}}),
				std::pair<int, int>({0, -1})
			)
		},
		{
			"east",
			CloseEnd(
				std::vector<std::pair<int, int>>({{-1, 0}, {-1, -1}, {0, -1}, {1, -1}, {1, 0}}),
				std::vector<std::pair<int, int>>({{0, 0}}),
				std::pair<int, int>({0, 1})
			)
		}
	});
}

Dungeon::~Dungeon()
{
	delete cell;
	for (std::map<int, DungeonRoom*>::iterator i = rooms.begin(); i != rooms.end(); i = rooms.erase(i))
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
			if ((cell->get(r, c) & ROOM) || ((i == 0 || j == 0) && random_in_range(0, 1)))
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

	rooms[room_id] = new DungeonRoom(room_id, r1, c1, r1, r2, c1, c2, h, w, h * w);

	for (int r = r1 - 1; r <= r2 + 1; r++)
	{
		if (!(cell->get(r, c1 - 1) & (ROOM | ENTRANCE)))
		{
			cell->set(r, c1 - 1, cell->get(r, c1 - 1) | PERIMETER);
		}
		if (!(cell->get(r, c2 + 1) & (ROOM | ENTRANCE)))
		{
			cell->set(r, c2 + 1, cell->get(r, c1 - 1) | PERIMETER);
		}
	}
	for (int c = c1 - 1; c <= c2 + 1; c++)
	{
		if (!(cell->get(r1 - 1, c) & (ROOM | ENTRANCE)))
		{
			cell->set(r1 - 1, c, cell->get(r1 - 1, c) | PERIMETER);
		}
		if (!(cell->get(r2 + 1, c) & (ROOM | ENTRANCE)))
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
			height = random_in_range(0, std::min(std::max(n_i - room_base - i, 0), room_radix)) + room_base;
		}
		else
		{
			height = random_in_range(0, room_radix) + room_base;
		}
	}
	if (width == 0)
	{
		if (j)
		{
			width = random_in_range(0, std::min(std::max(n_j - room_base - j, 0), room_radix)) + room_base;
		}
		else
		{
			width = random_in_range(0, room_radix) + room_base;
		}
	}
	if (i == 0)
	{
		i = random_in_range(0, n_i - height);
	}
	if (j == 0)
	{
		j = random_in_range(0, n_j - width);
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

void Dungeon::open_rooms()
{
	for (int id = 1; id <= n_rooms; id++)
	{
		open_room(rooms[id]);
	}
	connections.clear();
}

void Dungeon::open_room(DungeonRoom* room)
{
	std::vector<Sill> sills = door_sills(room);
	if (sills.empty())
	{
		return;
	}
	int n_opens = alloc_opens(room);

	for (int i = 0; i < n_opens && !sills.empty(); i++)
	{
		Sill sill = sills.back();
		sills.pop_back();
		int door_r = sill.door_r;
		int door_c = sill.door_c;
		int door_cell = cell->get(door_r, door_c);
		if (door_cell & DOORSPACE)
		{
			i--;
			continue;
		}

		int out_id = sill.out_id;
		if (out_id)
		{
			std::pair<int, int> pair = std::make_pair(std::min(room->id, out_id), std::max(room->id, out_id));
			if (std::find(connections.begin(), connections.end(), pair) != connections.end())
			{
				i--;
				continue;
			}
			connections.push_back(pair);
		}
		int open_r = sill.sill_r;
		int open_c = sill.sill_c;
		std::string open_dir = sill.dir;
		for (int x = 0; x < 3; x++)
		{
			int r = open_r + (di[open_dir] * x);
			int c = open_c + (dj[open_dir] * x);

			cell->set(r, c, cell->get(r, c) & ~PERIMETER);
			cell->set(r, c, cell->get(r, c) | ENTRANCE);
		}
		int door_type = door_type();
		std::string key;
		std::string type;

		switch (door_type)
		{
		case ARCH:
			cell->set(door_r, door_c, cell->get(door_r, door_c) | door_type);
			key = "arch";
			type = "Archway";
			break;
		case DOOR:
			cell->set(door_r, door_c, cell->get(door_r, door_c) | door_type | ('o' << 24));
			key = "open";
			type = "Unlocked Door";
			break;
		case LOCKED:
			cell->set(door_r, door_c, cell->get(door_r, door_c) | door_type | ('x' << 24));
			key = "lock";
			type = "Locked Door";
			break;
		case TRAPPED:
			cell->set(door_r, door_c, cell->get(door_r, door_c) | door_type | ('t' << 24));
			key = "trap";
			type = "Trapped Door";
			break;
		case SECRET:
			cell->set(door_r, door_c, cell->get(door_r, door_c) | door_type | ('s' << 24));
			key = "secret";
			type = "Secret Door";
			break;
		case PORTC:
			cell->set(door_r, door_c, cell->get(door_r, door_c) | door_type | ('#' << 24));
			key = "portc";
			type = "Portcullis";
			break;
		default:
			break;
		}
		room->door[open_dir].push_back(Door(door_r, door_c, key, type, out_id));
	}
}

std::vector<Sill> Dungeon::door_sills(DungeonRoom* room)
{
	std::vector<Sill> list = std::vector<Sill>();
	if (room->north > 2)
	{
		for (int c = room->west; c <= room->east; c += 2)
		{
			std::optional<Sill> sill = check_sill(room, room->north, c, "north");
			if (sill.has_value())
			{
				list.push_back(sill.value());
			}
		}
	}
	if (room->south <= n_rows - 3)
	{
		for (int c = room->west; c <= room->east; c += 2)
		{
			std::optional<Sill> sill = check_sill(room, room->south, c, "south");
			if (sill.has_value())
			{
				list.push_back(sill.value());
			}
		}
	}
	if (room->west >= 3)
	{
		for (int r = room->north; r <= room->south; r += 2)
		{
			std::optional<Sill> sill = check_sill(room, r, room->west, "west");
			if (sill.has_value())
			{
				list.push_back(sill.value());
			}
		}
	}
	if (room->east <= n_cols - 3)
	{
		for (int r = room->north; r <= room->south; r += 2)
		{
			std::optional<Sill> sill = check_sill(room, r, room->east, "east");
			if (sill.has_value())
			{
				list.push_back(sill.value());
			}
		}
	}
	return shuffle(list);
}

std::optional<Sill> Dungeon::check_sill(DungeonRoom* room, int sill_r, int sill_c, std::string dir)
{
	int door_r = sill_r + di[dir];
	int door_c = sill_c + dj[dir];
	int door_cell = cell->get(door_r, door_c);
	if (!(door_cell & PERIMETER) || (door_cell & BLOCK_ROOM))
	{
		return std::nullopt;
	}
	int out_r = door_r + di[dir];
	int out_c = door_c + dj[dir];
	int out_cell = cell->get(out_r, out_c);
	if (out_cell & BLOCKED)
	{
		return std::nullopt;
	}
	int out_id = 0;
	if (out_cell & ROOM)
	{
		out_id = (out_cell & ROOM_ID) >> 6;
		if (out_id == room->id)
		{
			return std::nullopt;
		}
	}
	return std::optional<Sill>(Sill(sill_r, sill_c, dir, door_r, door_c, out_id));
}

template <typename T>
std::vector<T> Dungeon::shuffle(std::vector<T> sills)
{
	std::vector<int> rand_ints = std::vector<int>();
	std::vector<int> indices = std::vector<int>();

	for (int i = 0; i < sills.size(); i++)
	{
		rand_ints.push_back(random_in_range(0, 100));
		indices.push_back(i);
	}
	std::sort(indices.begin(), indices.end(), [&rand_ints](int a, int b) { return rand_ints[a] < rand_ints[b]; });
	std::vector<T> list = std::vector<T>();

	for (int i = 0; i < sills.size(); i++)
	{
		list.push_back(sills[indices[i]]);
	}
	return list;
}

int Dungeon::alloc_opens(DungeonRoom* room)
{
	int room_h = (room->south - room->north) / 2 + 1;
	int room_w = (room->east - room->west) / 2 + 1;
	int flumph = int(sqrt(room_h * room_w));
	return flumph + random_in_range(0, flumph);
}

int Dungeon::door_type()
{
	int i = random_in_range(0, 110);
	if (i < 15)
	{
		return ARCH;
	}
	else if (i < 60)
	{
		return DOOR;
	}
	else if (i < 75)
	{
		return LOCKED;
	}
	else if (i < 90)
	{
		return TRAPPED;
	}
	else if (i < 100)
	{
		return SECRET;
	}
	else
	{
		return PORTC;
	}
}

void Dungeon::label_rooms()
{
	for (int id = 1; id <= n_rows; id++)
	{
		DungeonRoom* room = rooms[id];
		std::string label = std::to_string(room->id);
		int len = label.length();
		int label_r = int((room->north + room->south) / 2);
		int label_c = int((room->east + room->west - len) / 2 + 1);

		for (int c = 0; c < len; c++)
		{
			char chacha = label.at(c);
			cell->set(label_r, label_c + c, cell->get(label_r, label_c + c) | (chacha << 24));
		}
	}
}

void Dungeon::corridors()
{
	for (int i = 1; i < n_i; i++)
	{
		int r = i * 2 + 1;
		for (int j = 1; j < n_j; j++)
		{
			int c = j * 2 + 1;
			if (cell->get(r, c) & CORRIDOR)
			{
				continue;
			}
			tunnel(i, j, std::nullopt);
		}
	}
}

void Dungeon::tunnel(int i, int j, std::optional<std::string> last_dir)
{
	std::vector<std::string> dirs = shuffle(keys_of(dj));

	for (std::string dir : dirs)
	{
		if (open_tunnel(i, j, dir))
		{
			int next_i = i + di[dir];
			int next_j = j + dj[dir];
			tunnel(next_i, next_j, dir);
		}
	}
}

template <typename T>
std::vector<std::string> Dungeon::keys_of(std::map<std::string, T> the_map)
{
	std::vector<std::string> keys = std::vector<std::string>();
	for (const auto& pair : the_map)
	{
		keys.push_back(pair.first);
	}
	return keys;
}

bool Dungeon::open_tunnel(int i, int j, std::string dir)
{
	int this_r = i * 2 + 1;
	int this_c = j * 2 + 1;
	int next_r = (i + di[dir]) * 2 + 1;
	int next_c = (j + dj[dir]) * 2 + 1;
	int mid_r = (this_r + next_r) / 2;
	int mid_c = (this_c + next_c) / 2;

	if (sound_tunnel(mid_r, mid_c, next_r, next_c))
	{
		delve_tunnel(this_r, this_c, next_r, next_c);
		return true;
	}
	return false;
}

bool Dungeon::sound_tunnel(int mid_r, int mid_c, int next_r, int next_c)
{
	if ((next_r < 0 || next_r > n_rows) || (next_c < 0 || next_c > n_cols))
	{
		return false;
	}
	for (int r = std::min(mid_r, next_r); r <= std::max(mid_r, next_r); r++)
	{
		for (int c = std::min(mid_c, next_c); c <= std::max(mid_c, next_c); c++)
		{
			if (cell->get(r, c) & BLOCK_CORR)
			{
				return false;
			}
		}
	}
	return true;
}

void Dungeon::delve_tunnel(int mid_r, int mid_c, int next_r, int next_c)
{
	for (int r = std::min(mid_r, next_r); r <= std::max(mid_r, next_r); r++)
	{
		for (int c = std::min(mid_c, next_c); c <= std::max(mid_c, next_c); c++)
		{
			cell->set(r, c, cell->get(r, c) & ~ENTRANCE);
			cell->set(r, c, cell->get(r, c) | CORRIDOR);
		}
	}
}

void Dungeon::clean_dungeon()
{
	if (deadends_percent)
	{
		collapse_tunnels(deadends_percent, close_ends);
	}
	fix_doors();
	//TODO: empty_blocks();
}

void Dungeon::collapse_tunnels(int p, std::map<std::string, CloseEnd> xc)
{
	for (int i = 0; i < n_i; i++)
	{
		int r = i * 2 + 1;
		for (int j = 0; j < n_j; j++)
		{
			int c = j * 2 + 1;
			if (!(cell->get(r, c) & OPENSPACE) || (cell->get(r, c) & STAIRS) || !((p == 100) || random_in_range(0, 100)
				< p))
			{
				continue;
			}
			collapse(r, c, xc);
		}
	}
}

void Dungeon::collapse(int r, int c, std::map<std::string, CloseEnd> xc)
{
	if (!(cell->get(r, c) & OPENSPACE))
	{
		return;
	}
	for (const auto& dir : keys_of(xc))
	{
		if (check_tunnel(r, c, xc[dir]))
		{
			for (const auto& p : xc[dir].close)
			{
				cell->set(r + p.first, c + p.second, cell->get(r + p.first, c + p.second) | CORRIDOR);
			}
			std::pair<int, int> recurse = xc[dir].recurse;
			collapse(r + recurse.first, c + recurse.second, xc);
		}
	}
}

bool Dungeon::check_tunnel(int r, int c, CloseEnd xc)
{
	for (const auto& p : xc)
	{
		if (cell->get(r + p.first, c + p.second) & OPENSPACE)
		{
			return false;
		}
	}
	return true;
}

void Dungeon::fix_doors()
{
	TwoDArray<bool> fixed = TwoDArray<bool>(n_cols, n_rows);
	for (const auto& room : rooms)
	{
		for (const auto& dir : room.second->door)
		{
			std::vector<Door> shiny = std::vector<Door>();
			for (const auto& door : dir.second)
			{
				int door_r = door.row;
				int door_c = door.col;
				int door_cell = cell->get(door_r, door_c);
				if (!(door_cell & OPENSPACE))
				{
					continue;
				}

				if (fixed.get(door_r, door_c))
				{
					shiny.push_back(door);
				}
				else
				{
					int out_id = door.out_id;
					if (out_id)
					{
						rooms[out_id]->door[opposite[dir.first]].push_back(door);
					}
					shiny.push_back(door);
					fixed.set(door_r, door_c, true);
				}
			}
			if (!shiny.empty())
			{
				room.second->door[dir.first] = shiny;
				for (const auto& door : shiny)
				{
					doors.push_back(door);
				}
			} else
			{
				room.second->door[dir.first].clear();
			}
		}
	}
}


DungeonGenerator::DungeonGenerator()
{
	Dungeon* dungeon = new Dungeon(39, 39, 3, 9, std::time(nullptr), 50);
	dungeon->init_cells();
	dungeon->emplace_rooms();
	dungeon->open_rooms();
	dungeon->label_rooms();
	dungeon->corridors();
	dungeon->clean_dungeon();
}

DungeonGenerator::~DungeonGenerator()
{
}
