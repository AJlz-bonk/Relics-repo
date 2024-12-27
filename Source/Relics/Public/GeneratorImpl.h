#pragma once
#include "RoomImpl.h"
#include "TwoDArray.h"

class GeneratorImpl
{
    TwoDArray grid;
    const int size;
    const int room_min;
    const int room_max;
    const int gap;
    RandomGenerator rg;
    std::vector<RoomImpl> rooms;

    void round();
    bool placeStuff();
    bool openSpace() const;
    bool placeThing(char id);

public:
    GeneratorImpl(int size, int room_min, int room_max, int gap,
              int seed = RandomGenerator().getRandom());
    ~GeneratorImpl();
    bool generate();
    [[nodiscard]] const std::vector<RoomImpl>& getRooms() const;
    RandomGenerator& getRandomGenerator();
    friend inline std::ostream& operator<<(std::ostream& os, const GeneratorImpl& data);
};

inline std::ostream& operator<<(std::ostream& os, const GeneratorImpl& data)
{
    os << "seed: " << data.rg.getSeed() << std::endl;
    os << data.grid;
    return os;
}
