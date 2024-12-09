#pragma once

template <typename T>
class TwoDArray
{
	int rows;
	int cols;
	T nil;
	T* data;

public:
	TwoDArray(int r, int c);

	~TwoDArray();

	T& get(int i, int j);

	void set(int i, int j, T val);

	void fill(const T& val);

	void print();
};

#include <iomanip>
#include <iostream>


template <typename T>
TwoDArray<T>::TwoDArray(int r, int c) : rows(r), cols(c), data(new T[r * c])
{
}

template <typename T>
TwoDArray<T>::~TwoDArray()
{
	delete[] data;
}

template <typename T>
T& TwoDArray<T>::get(int i, int j)
{
	if (0 <= i && i < rows && 0 <= j && j < cols)
	{
		return data[i * cols + j];
	}
	return nil;
}

template <typename T>
void TwoDArray<T>::set(int i, int j, T val)
{
	if (0 <= i && i < rows && 0 <= j && j < cols)
	{
		data[i * cols + j] = val;
		return;
	}
	return;
}

template <typename T>
void TwoDArray<T>::fill(const T& val)
{
	for (int i = 0; i < rows; i++)
	{
		for (int j = 0; j < cols; j++)
		{
			data[i * cols + j] = val;
		}
	}
}

template <typename T>
void TwoDArray<T>::print()
{
	std::cout << std::hex;
	for (int i = 0; i < rows; i++)
	{
		for (int j = 0; j < cols; j++)
		{
			std::cout << std::setw(8) << data[i * cols + j] << " ";
		}
		std::cout << std::endl;
	}
	std::cout << std::dec << std::endl;
}
