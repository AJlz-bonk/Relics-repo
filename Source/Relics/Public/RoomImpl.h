#pragma once
#include <vector>

#include "TwoDArray.h"
#include "Relics/Utils/Utils.h"

class RoomImpl {

    unsigned int id;
    unsigned int row;
    unsigned int col;
    unsigned int width;
    unsigned int height;
    std::vector<std::pair<int, int>> walls;
    std::vector<std::pair<int, int>> interior_walls;
    std::set<std::pair<int, int>> doors;

    void drawLine(bool isVert, std::pair<int, int>* wall, std::pair<int, int>* next, TwoDArray& grid) const;
    void reshapeL(RandomGenerator& rg);
    void reshapeU(RandomGenerator& rg);
    void reshapeO(RandomGenerator& rg);
    void addDoors(RandomGenerator& rg);
    void addDoorToWall(RandomGenerator& rg, bool isVert, std::pair<int, int> next, std::pair<int, int>* wall, bool hasDoor);

    public:
    RoomImpl(int id, int row, int col, int width, int height, RandomGenerator& rg);
    RoomImpl();
    void draw(TwoDArray& grid);
    unsigned int getId() const;
    unsigned int getRow() const;
    unsigned int getCol() const;
    unsigned int getWidth() const;
    unsigned int getHeight() const;
    std::set<std::pair<int, int>>& getDoors();
    std::vector<std::pair<int, int>>& getWalls();
    std::vector<std::pair<int, int>>& getInteriorWalls();
};
