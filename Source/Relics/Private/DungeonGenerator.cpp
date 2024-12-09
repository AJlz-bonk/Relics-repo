#include "DungeonGenerator.h"
#include "../Relics.h"
#include "Components/SceneComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include <cstdlib>
#include <ctime>
#include <map>
#include <optional>
#include <string>
#include <utility>
#include <vector>
#include <cmath>
#include <algorithm>
#include <iostream>
#include <iomanip>
#include <random>
#include <sstream>
#include "Relics/Utils/Utils.h"

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

CloseEnd::CloseEnd(std::vector<std::pair<int, int>> walled, std::vector<std::pair<int, int>> close,
                   std::pair<int, int> recurse)
	: walled(std::move(walled)), close(std::move(close)), recurse(recurse)
{
}

CloseEnd::CloseEnd(const CloseEnd& other)
	: walled(other.walled), close(other.close), recurse(other.recurse)
{
}

CloseEnd::CloseEnd()
	: walled(std::vector<std::pair<int, int>>()), close(std::vector<std::pair<int, int>>()),
	  recurse(std::make_pair(0, 0))
{
}

Sill::Sill(int sill_r, int sill_c, std::string dir, int door_r, int door_c, unsigned int out_id)
	: sill_r(sill_r), sill_c(sill_c), dir(std::move(dir)), door_r(door_r), door_c(door_c), out_id(out_id)
{
}

DungeonRoom::DungeonRoom(unsigned int id, int row, int col, int north, int south, int west, int east, int height,
                         int width,
                         int area)
	: id(id), row(row), col(col), north(north), south(south), west(west), east(east), height(height), width(width),
	  area(area), doors(std::map<std::string, std::vector<Door>>())
{
}

void DungeonRoom::print(TwoDArray<char>& out)
{
	out.set(row, col, '+');
	out.set(row + height - 1, col, '+');
	out.set(row, col + width - 1, '+');
	out.set(row + height - 1, col + width - 1, '+');

	for (int i = 1; i < height - 1; i++)
	{
		out.set(row + i, col, '|');
		out.set(row + i, col + width - 1, '|');
	}
	for (int j = 1; j < width - 1; j++)
	{
		out.set(row, col + j, '-');
		out.set(row + height - 1, col + j, '-');
	}

	int tid = id;
	int count = 0;
	while (tid > 0)
	{
		tid = tid / 10;
		count++;
	}

	tid = id;
	count--;
	while (tid > 0)
	{
		out.set(row + height / 2, col + width / 2 + count, char(tid % 10) + '0');
		tid = tid / 10;
		count--;
	}

	for (const auto& entry : doors)
	{
		for (const auto& door : entry.second)
		{
			door.print(out, entry.first);
		}
	}
}

ARoom* DungeonRoom::build(UWorld* world, ADungeonGenerator* generator)
{
	FVector spawnLocation(row * 100.f, col * 100.f, 0.f);

	int alt = random_in_range(4, 7);

	// Spawn the actor.
	ARoom* spawnedRoom = world->SpawnActorDeferred<ARoom>(ARoom::StaticClass(), FTransform(spawnLocation), generator,
	                                                      nullptr);
	if (spawnedRoom)
	{
		//makes a list of doors with coordinates relative to rooms instead of world origin
		std::map<std::string, std::vector<Door>> doorsies = std::map<std::string, std::vector<Door>>();
		for (const auto& entry : doors)
		{
			for (const auto& door : entry.second)
			{
				doorsies[entry.first].push_back(Door(door.row - row, door.col - col, door.key, door.type, door.out_id));
			}
		}
		spawnedRoom->height = height;
		spawnedRoom->width = width;
		spawnedRoom->alt = alt;
		spawnedRoom->doors = doorsies;
		UE_LOG(LogTemp, Log, TEXT("Spawned actor %s successfully!"), *spawnedRoom->GetName());

		return spawnedRoom;
	}
	UE_LOG(LogTemp, Error, TEXT("Failed to spawn actor."));

	return nullptr;
}

Dungeon::Dungeon(int n_rows, int n_cols, int room_min, int room_max, int seed, int deadends_percent, int perimeter_size)
	: n_rows(n_rows / 2 * 2), n_cols(n_cols / 2 * 2), n_rooms(0), room_min(room_min), room_max(room_max), seed(seed),
	  deadends_percent(deadends_percent), perimeter_size(perimeter_size)
{
	this->n_i = this->n_rows / 2;
	this->n_j = this->n_cols / 2;
	this->max_row = this->n_rows - 1;
	this->max_col = this->n_cols - 1;
	this->room_base = (this->room_min + 1) / 2;
	this->room_radix = (this->room_max - this->room_min) / 2 + 1;
	this->cell = new TwoDArray<unsigned int>(this->n_rows, this->n_cols);
	this->rooms = std::map<unsigned int, DungeonRoom*>();
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
	for (auto i = rooms.begin(); i != rooms.end(); i = rooms.erase(i))
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

void Dungeon::round_mask() const
{
	int center_r = n_rows / 2;
	int center_c = n_cols / 2;

	for (int i = 0; i < n_rows; i++)
	{
		for (int j = 0; j < n_cols; j++)
		{
			double d = sqrt(pow(i - center_r, 2) + pow(j - center_c, 2));
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
			if (cell->get(r, c) & ROOM || ((i == 0 || j == 0) && random_in_range(0, 1)))
			{
				continue;
			}
			emplace_room(i, j);
		}
	}
}

void Dungeon::emplace_room(int i, int j)
{
	int width = 0;
	int height = 0;
	set_room(i, j, width, height);

	int r1 = i * 2 + 1;
	int c1 = j * 2 + 1;
	int r2 = (i + height) * 2 - 1;
	int c2 = (j + width) * 2 - 1;

	if (r1 < perimeter_size || r2 >= max_row - (perimeter_size - 1) || (
		c1 < perimeter_size || c2 >= max_col - (perimeter_size - 1)))
	{
		return;
	}

	bool blocked;
	std::map<unsigned int, int> hit = std::map<unsigned int, int>();
	sound_room(r1, c1, r2, c2, blocked, hit);
	if (blocked)
	{
		return;
	}
	unless(hit.empty())
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
	int h = r2 - r1 + 1;
	int w = c2 - c1 + 1;

	rooms[room_id] = new DungeonRoom(room_id, r1, c1, r1, r2, c1, c2, h, w, h * w);

	for (int n = 1; n < perimeter_size; n++)
	{
		for (int r = r1 - n; r <= r2 + n; r++)
		{
			unless(cell->get(r, c1 - n) & (ROOM | ENTRANCE))
			{
				cell->set(r, c1 - n, cell->get(r, c1 - n) | PERIMETER);
			}
			unless(cell->get(r, c2 + n) & (ROOM | ENTRANCE))
			{
				cell->set(r, c2 + n, cell->get(r, c2 + n) | PERIMETER);
			}
		}
		for (int c = c1 - n + 1; c <= c2 + n - 1; c++)
		{
			unless(cell->get(r1 - n, c) & (ROOM | ENTRANCE))
			{
				cell->set(r1 - n, c, cell->get(r1 - n, c) | PERIMETER);
			}
			unless(cell->get(r2 + n, c) & (ROOM | ENTRANCE))
			{
				cell->set(r2 + n, c, cell->get(r2 + n, c) | PERIMETER);
			}
		}
	}
}

void Dungeon::set_room(int& i, int& j, int& width, int& height) const
{
	if (i)
	{
		height = random_in_range(0, std::min(std::max(n_i - room_base - i, 0), room_radix)) + room_base;
	}
	else
	{
		height = random_in_range(0, room_radix) + room_base;
	}
	if (j)
	{
		width = random_in_range(0, std::min(std::max(n_j - room_base - j, 0), room_radix)) + room_base;
	}
	else
	{
		width = random_in_range(0, room_radix) + room_base;
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

void Dungeon::sound_room(int r1, int c1, int r2, int c2, bool& blocked, std::map<unsigned int, int>& hit) const
{
	blocked = false;
	for (int r = r1; r <= r2; r++)
	{
		for (int c = c1; c <= c2; c++)
		{
			if (cell->get(r, c) & (BLOCKED | PERIMETER))
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
	//connections.clear();
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
		unsigned int door_cell = cell->get(door_r, door_c);
		if (door_cell & DOORSPACE)
		{
			i--;
			continue;
		}

		unsigned int out_id = sill.out_id;
		if (out_id)
		{
			std::pair<int, int> pair = std::make_pair(std::min(room->id, out_id), std::max(room->id, out_id));
			if (std::ranges::find(connections.begin(), connections.end(), pair) != connections.end())
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
		int door_type = get_door_type();
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
		room->doors[open_dir].emplace_back(door_r, door_c, key, type, out_id);
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

std::optional<Sill> Dungeon::check_sill(DungeonRoom* room, int sill_r, int sill_c, const std::string& dir) const
{
	int door_r = sill_r + di[dir];
	int door_c = sill_c + dj[dir];
	unsigned int door_cell = cell->get(door_r, door_c);
	unless(door_cell & PERIMETER || door_cell & BLOCK_ROOM)
	{
		return std::nullopt;
	}
	int out_r = door_r + di[dir];
	int out_c = door_c + dj[dir];
	unsigned int out_cell = cell->get(out_r, out_c);
	if (out_cell & BLOCKED)
	{
		return std::nullopt;
	}
	unsigned int out_id = 0;
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

int Dungeon::get_door_type()
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
	for (int id = 1; id <= n_rooms; id++)
	{
		DungeonRoom* room = rooms[id];
		std::string label = std::to_string(room->id);
		unsigned int len = label.length();
		int label_r = int((room->north + room->south) / 2);
		int label_c = int((room->east + room->west - len) / 2 + 1);

		for (unsigned int c = 0; c < len; c++)
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

void Dungeon::tunnel(int i, int j, const std::optional<std::string>& last_dir)
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

bool Dungeon::open_tunnel(int i, int j, const std::string& dir)
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

bool Dungeon::sound_tunnel(int mid_r, int mid_c, int next_r, int next_c) const
{
	if (next_r < 0 || next_r > n_rows || next_c < 0 || next_c > n_cols)
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

void Dungeon::delve_tunnel(int mid_r, int mid_c, int next_r, int next_c) const
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
	empty_blocks();
}

void Dungeon::collapse_tunnels(int p, std::map<std::string, CloseEnd>& xc)
{
	for (int i = 0; i < n_i; i++)
	{
		int r = i * 2 + 1;
		for (int j = 0; j < n_j; j++)
		{
			int c = j * 2 + 1;

			unless(cell->get(r, c) & OPENSPACE)
			{
				continue;
			}
			unless(p == 100 || random_in_range(0, 100) < p)
			{
				continue;
			}
			collapse(r, c, xc);
		}
	}
}

void Dungeon::collapse(int r, int c, std::map<std::string, CloseEnd>& xc)
{
	unless(0 <= r && r < max_row && 0 <= c && c < max_col)
	{
		return;
	}
	unless(cell->get(r, c) & OPENSPACE)
	{
		return;
	}
	for (const std::string& dir : keys_of(xc))
	{
		if (check_tunnel(r, c, xc[dir]))
		{
			for (const auto& p : xc[dir].close)
			{
				cell->set(r + p.first, c + p.second, NOTHING);
			}
			std::pair<int, int> recurse = xc[dir].recurse;
			collapse(r + recurse.first, c + recurse.second, xc);
		}
	}
}

bool Dungeon::check_tunnel(int r, int c, const CloseEnd& xc) const
{
	for (const auto& p : xc.walled)
	{
		if (0 <= r + p.first && r + p.first < max_row && 0 <= c + p.second && c + p.second < max_col)
		{
			if (cell->get(r + p.first, c + p.second) & OPENSPACE)
			{
				return false;
			}
		}
	}
	return true;
}

void Dungeon::fix_doors()
{
	TwoDArray<bool> fixed = TwoDArray<bool>(n_cols, n_rows);
	for (const auto& room : rooms)
	{
		for (const auto& dir : room.second->doors)
		{
			std::vector<Door> shiny = std::vector<Door>();
			for (const auto& door : dir.second)
			{
				int door_r = door.row;
				int door_c = door.col;
				unsigned int door_cell = cell->get(door_r, door_c);
				unless(door_cell & OPENSPACE)
				{
					continue;
				}

				if (fixed.get(door_r, door_c))
				{
					shiny.push_back(door);
				}
				else
				{
					unsigned int out_id = door.out_id;
					if (out_id)
					{
						rooms[out_id]->doors[opposite[dir.first]].push_back(door);
					}
					shiny.push_back(door);
					fixed.set(door_r, door_c, true);
				}
			}
			if (shiny.empty())
			{
				room.second->doors[dir.first].clear();
			}
			else
			{
				room.second->doors[dir.first] = shiny;
			}
		}
	}
}

void Dungeon::empty_blocks() const
{
	for (int r = 0; r < n_rows; r++)
	{
		for (int c = 0; c < n_cols; c++)
		{
			if (cell->get(r, c) & BLOCKED)
			{
				cell->set(r, c, NOTHING);
			}
		}
	}
}

void Dungeon::print()
{
	TwoDArray<char> out = TwoDArray<char>(n_rows, n_cols);
	out.fill(' ');
	for (const auto& entry : rooms)
	{
		entry.second->print(out);
	}

	for (int i = n_rows - 1; i >= 0; i--)
	{
		for (int j = 0; j < n_cols; j++)
		{
			std::cout << out.get(i, j);
		}
		std::cout << std::endl;
	}
}

std::string Dungeon::toString()
{
	TwoDArray<char> out = TwoDArray<char>(n_rows, n_cols);
	out.fill(' ');
	for (const auto& entry : rooms)
	{
		entry.second->print(out);
	}

	std::stringstream ss = std::stringstream();
	for (int i = n_rows - 1; i >= 0; i--)
	{
		for (int j = 0; j < n_cols; j++)
		{
			ss << out.get(i, j);
		}
		ss << std::endl;
	}

	return ss.str();
}

std::vector<ARoom*> Dungeon::build(UWorld* world, ADungeonGenerator* generator)
{
	std::vector<ARoom*> roomsies = std::vector<ARoom*>();
	for (const auto& entry : rooms)
	{
		ARoom* room = entry.second->build(world, generator);
		unless(room == nullptr)
		{
			roomsies.push_back(room);
		}
	}
	return roomsies;
}

ADungeonGenerator::ADungeonGenerator()
	: roomsies(std::vector<ARoom*>()), dungeon(nullptr), size(64), room_min(3), room_max(5), seed(getRandomInt()),
	  perimeter_size(5)
{
	PrimaryActorTick.bCanEverTick = false;

	USceneComponent* SceneComponent = CreateDefaultSubobject<USceneComponent>(TEXT("SceneComponent"));
	SetRootComponent(SceneComponent);

	blocks = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("Walls"));
	blocks->SetupAttachment(SceneComponent);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> cubeMesh(TEXT("/Game/LevelPrototyping/Meshes/SM_Cube"));
	if (cubeMesh.Succeeded())
	{
		blocks->SetStaticMesh(cubeMesh.Object);
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("Could not find the cube mesh"));
	}
}

ADungeonGenerator::~ADungeonGenerator()
{
	UE_LOG(LogTemp, Warning, TEXT("ADungeonGenerator destructor called"));
	/*
	delete dungeon;
	for (auto entry : roomsies)
	{
		entry->Destroy();
	}
	roomsies.clear();
	*/
}

void ADungeonGenerator::init()
{
	if (dungeon != nullptr)
	{
		delete dungeon;
	}

	dungeon = new Dungeon(size, size, room_min, room_max, seed, 0, perimeter_size);

	UE_LOG(LogRelics, Log, TEXT("init_cells"));
	dungeon->init_cells();

	UE_LOG(LogRelics, Log, TEXT("emplace_rooms"));
	dungeon->emplace_rooms();

	UE_LOG(LogRelics, Log, TEXT("open_rooms"));
	dungeon->open_rooms();

	UE_LOG(LogRelics, Log, TEXT("label_rooms"));
	dungeon->label_rooms();

	UE_LOG(LogRelics, Log, TEXT("corridors"));
	dungeon->corridors();

	UE_LOG(LogRelics, Log, TEXT("clean_dungeon"));
	dungeon->clean_dungeon();

	UE_LOG(LogRelics, Log, TEXT("done init"));
}

void ADungeonGenerator::print_dungeon()
{
	if (dungeon != nullptr)
	{
		dungeon->print();
	}
}

void ADungeonGenerator::buildBasePlate()
{
	FMatrix transformMatrix = FMatrix(
		FPlane(size * 1.0f, 0.0f, 0.0f, 0.0f),
		FPlane(0.0f, size * 1.0f, 0.0f, 0.0f),
		FPlane(0.0f, 0.0f, 1.0f, 0.0f),
		FPlane(0.0f, 0.0f, -100.0f, 1.0f)
	);

	blocks->ClearInstances();
	blocks->AddInstance(FTransform(transformMatrix));
}

void ADungeonGenerator::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	UWorld* world = GetWorld();

	init();

	//clears the list of room actors so that new ones can be created
	for (auto entry : roomsies)
	{
		entry->Destroy();
	}
	roomsies.clear();

	roomsies = dungeon->build(world, this);

	buildBasePlate();

	delete dungeon;
	dungeon = nullptr;
}