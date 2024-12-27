#pragma once
#include <iostream>
#include <set>
#include <string>

#include "Utils.h"

class TwoDArray
{
    const int row;
    const int col;
    int overflow;
    std::string data;
    const char empty;
    const std::set<char> nonBlocking = {'X'};

public:
    TwoDArray(const int row, const int col, const char empty = '-') :
        row(row), col(col), overflow(10), data(row * (col + 1), empty), empty(empty)
    {
        const int max = static_cast<int>(data.size());
        for (int i = col; i < max; i += col + 1)
        {
            data[i] = '\n';
        }
    }

    TwoDArray()
        : row(0), col(0), overflow(10), data(0, '-'), empty('-')
    {
    }

    [[nodiscard]] char get(const int r, const int c, const char defaultValue = '\0') const
    {
        if (r >= 0 && r < row && c >= 0 && c < col)
        {
            return data[(row - r - 1) * (col + 1) + c];
        }
        if (defaultValue == '\0')
        {
            std::cout << "attempted to get out of bounds" << std::endl;
        }
        return defaultValue;
    }

    void set(const int r, const int c, const char ch)
    {
        if (overflow < 0)
        {
            return;
        }
        if (r >= 0 && r < row && c >= 0 && c < col)
        {
            data[(row - r - 1) * (col + 1) + c] = ch;
        }
        else
        {
            std::cout << "attempted to set out of bounds" << std::endl;
            overflow--;
        }
    }

    [[nodiscard]] bool isEmpty(const int r, const int c, const int s, const int gap) const
    {
        if (s == 0)
        {
            return get(r, c) == empty;
        }
        if (r + s >= row || c + s >= col)
        {
            return false;
        }
        for (int i = -gap; i < s + gap; i++)
        {
            const char a = get(r + s, c + i, 'X');
            const char b = get(r + i, c + s, 'X');
            if (a == empty && b == empty)
            {
                continue;
            }
            if (nonBlocking.contains(a) && (i < 0 || i > s))
            {
                continue;
            }
            if (nonBlocking.contains(b) && (i < 0 || i > s))
            {
                continue;
            }
            return false;
        }
        return true;
    }

    [[nodiscard]] bool isEmpty(const int r, const int c, const int w, const int h, const int gap)
    {
        for (int i = r - gap; i < r + h + gap; i++)
        {
            for (int j = c - gap; j < c + w + gap; j++)
            {
                const char a = get(i, j, 'X');
                if (a == empty)
                {
                    continue;
                }
                if (nonBlocking.contains(a) && ((i < r || i > r + h) || (j < c || j > c + w)))
                {
                    continue;
                }
                return false;
            }
        }
        return true;
    }

    void fill(const int r, const int c, const int w, const int h, const int gap, const char ch)
    {
        for (int i = r; i < r + h; i++)
        {
            for (int j = c; j < c + w; j++)
            {
                set(i, j, ch);
            }
        }
    }

    friend inline std::ostream& operator<<(std::ostream& os, const TwoDArray& data);
};

inline std::ostream& operator<<(std::ostream& os, const TwoDArray& data)
{
    os << data.data;
    return os;
}
