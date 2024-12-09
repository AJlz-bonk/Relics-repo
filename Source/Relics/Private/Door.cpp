#include "Door.h"

Door::Door(int row, int col, std::string key, std::string type, unsigned int out_id)
	: row(row), col(col), key(std::move(key)), type(std::move(type)), out_id(out_id)
{
}

void Door::print(TwoDArray<char>& out, std::string dir) const
{
	out.set(row, col, key.at(0));
}

void Door::build(UInstancedStaticMeshComponent* blocks, std::string dir) const
{
	unsigned int x = row;
	unsigned int y = col;

	if (dir == "north")
	{
		x++;
	}
	else if (dir == "south")
	{
		x--;
	}
	else if (dir == "west")
	{
		y++;
	}
	else if (dir == "east")
	{
		y--;
	}

	FMatrix transformMatrix = FMatrix(
		FPlane(1.0f, 0.0f, 0.0f, 0.0f),
		FPlane(0.0f, 1.0f, 0.0f, 0.0f),
		FPlane(0.0f, 0.0f, 1.0f, 0.0f),
		FPlane(x * 100.0f, y * 100.0f, 0.0f, 1.0f)
	);

	//blocks->AddInstance(FTransform(transformMatrix));
}
