#pragma once
#include <map>
#include <optional>
#include <string>
#include <vector>
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "DungeonGenerator.generated.h"

template<typename T>
class TwoDArray {
    int rows;
    int cols;
    T nil;
    T *data;

public:
    TwoDArray(int r, int c);

    ~TwoDArray();

    T &get(int i, int j);

    void set(int i, int j, T val);

    void fill(const T &val);
};

class CloseEnd {
public:
    std::vector<std::pair<int, int> > walled;
    std::vector<std::pair<int, int> > close;
    std::pair<int, int> recurse;

    CloseEnd(std::vector<std::pair<int, int> > walled, std::vector<std::pair<int, int> > close,
             std::pair<int, int> recurse);

    CloseEnd(const CloseEnd &other);

    CloseEnd();
};

class Door {
public:
    int row;
    int col;
    std::string key;
    std::string type;
    unsigned int out_id;

    Door(int row, int col, std::string key, std::string type, unsigned int out_id);

    void print(TwoDArray<char> & out, std::string dir) const;
};

class Sill {
public:
    int sill_r;
    int sill_c;
    std::string dir;
    int door_r;
    int door_c;
    unsigned int out_id;

    Sill(int sill_r, int sill_c, std::string dir, int door_r, int door_c, unsigned int out_id);
};

class DungeonRoom {
public:
    DungeonRoom(unsigned int id, int row, int col, int north, int south, int west, int east, int height, int width,
                int area);

    unsigned int id;
    int row;
    int col;
    int north;
    int south;
    int west;
    int east;
    int height;
    int width;
    int area;
    std::map<std::string, std::vector<Door> > doors;

    void print(TwoDArray<char>& out);
    
    void render(UInstancedStaticMeshComponent* floors);
};

class Dungeon {
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
    std::map<unsigned int, DungeonRoom *> rooms;
    std::vector<std::pair<int, int> > connections;
    TwoDArray<unsigned int> *cell;
    std::map<std::string, CloseEnd> close_ends;
    std::vector<Door> doors;

    void round_mask() const;

    void pack_rooms();

    void emplace_room(int i, int j);

    void set_room(int &i, int &j, int &width, int &height) const;

    void sound_room(int r1, int c1, int r2, int c2, bool &blocked, std::map<unsigned int, int> &hit) const;

    void open_room(DungeonRoom *room);

    std::vector<Sill> door_sills(DungeonRoom *room);

    std::optional<Sill> check_sill(DungeonRoom *room, int sill_r, int sill_c, const std::string &dir) const;

    template<typename T>
    std::vector<T> shuffle(std::vector<T> sills);

    static int alloc_opens(DungeonRoom *room);

    static int get_door_type();

    void tunnel(int i, int j, const std::optional<std::string> &last_dir);

    template<typename T>
    std::vector<std::string> keys_of(std::map<std::string, T> the_map);

    bool open_tunnel(int i, int j, const std::string &dir);

    bool sound_tunnel(int mid_r, int mid_c, int next_r, int next_c) const;

    void delve_tunnel(int mid_r, int mid_c, int next_r, int next_c) const;

    void collapse_tunnels(int p, std::map<std::string, CloseEnd> &close_end);

    void collapse(int r, int c, std::map<std::string, CloseEnd>& xc);

    bool check_tunnel(int r, int c, const CloseEnd &xc) const;

    void empty_blocks() const;

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

    void print();

    std::string toString();

    void render(UInstancedStaticMeshComponent* floors);
};

UCLASS()
class RELICS_API ADungeonGenerator : public AActor
{
    Dungeon *dungeon;
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ADungeonGenerator();
    
    ~ADungeonGenerator();
    
    void print_dungeon();
    
    virtual void OnConstruction(const FTransform &Transform) override;

    UPROPERTY(EditAnywhere)
    class UInstancedStaticMeshComponent* floors;

    UPROPERTY(EditAnywhere, meta = (ClampMin = 1))
    uint32 n_rows;

    UPROPERTY(EditAnywhere, meta = (ClampMin = 1))
    uint32 n_cols;

    UPROPERTY(EditAnywhere, meta = (ClampMin = 1))
    uint32 room_min;

    UPROPERTY(EditAnywhere, meta = (ClampMin = 1))
    uint32 room_max;

    UPROPERTY(EditAnywhere, meta = (ClampMin = 1))
    uint32 seed;
};
