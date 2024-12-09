#pragma once
#include <string>

#include "TwoDArray.h"

class Door
{
public:
	int row;
	int col;
	std::string key;
	std::string type;
	unsigned int out_id;

	Door(int row, int col, std::string key, std::string type, unsigned int out_id);

	void print(TwoDArray<char>& out, std::string dir) const;

	void build(UInstancedStaticMeshComponent* blocks, std::string dir) const;
};
