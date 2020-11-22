#pragma once

#include <cstring>
#include "Math/Vector2.hpp"

template <typename T>
class Grid
{
public:
    Grid()
        : _rows(0),
        _cols(0),
        _data(nullptr)
    {
        
    }

    explicit Grid(int rows, int cols, T* data)
        : _rows(rows),
          _cols(cols),
          _data(data)
    {
    }

    void Set(int rows, int cols, T* data)
    {
        _rows = rows;
        _cols = cols;
        _data = data;
    }

    const T* operator[](int row) const
    {
        return &_data[CalculateIndex(0, row)];
    }

    T* operator[](int row)
    {
        return &_data[CalculateIndex(0, row)];
    }

    T& operator[](const Vector2& position)
    {
        return _data[CalculateIndex(position.x, position.y)];
    }

    void FillWithZero()
    {
        memset(_data, 0, _rows * _cols * sizeof(T));
    }

    void FastCopyUnsafe(Grid& outGrid) const
    {
        Assert(Rows() == outGrid.Rows());
        Assert(Cols() == outGrid.Cols());

        memcpy(outGrid._data, _data, sizeof(T) * _rows * _cols);
    }

    template<typename U>
    void CopyToGridOfDifferentType(Grid<U>& outGrid) const;

    int Rows() const { return _rows; }
    int Cols() const { return _cols; }

    Vector2 Dimensions() const
    {
        return Vector2(Cols(), Rows());
    }

    T* Data() { return _data; }

protected:
    int CalculateIndex(int x, int y) const
    {
        return y * _cols + x;
    }

    int _rows;
    int _cols;
    T* _data;
};

template<typename T>
template<typename U>
void Grid<T>::CopyToGridOfDifferentType(Grid<U>& outGrid) const
{
    for(int i = 0; i < Rows(); ++i)
    {
        for(int j = 0; j < Cols(); ++j)
        {
            outGrid[i][j] = static_cast<U>(operator[](i)[j]);
        }
    }
}

template<typename T, int NumRows, int NumCols>
class FixedSizeGrid : public Grid<T>
{
public:
    FixedSizeGrid()
        : Grid<T>(NumRows, NumCols, _gridData)
    {

    }

    void FastCopyTo(FixedSizeGrid& outGrid) const
    {
        memcpy(outGrid._gridData, _gridData, sizeof(T) * NumRows * NumCols);
    }

    void operator=(const FixedSizeGrid& grid)
    {
        grid.FastCopyTo(*this);
    }

    bool operator==(const FixedSizeGrid& rhs) const;
    bool operator!=(const FixedSizeGrid& rhs) const { return !*this == rhs; }

private:
    T _gridData[NumRows * NumCols];
};

template <typename T, int NumRows, int NumCols>
bool FixedSizeGrid<T, NumRows, NumCols>::operator==(const FixedSizeGrid& rhs) const
{
    return memcmp(_gridData, rhs._gridData, sizeof(_gridData)) == 0;
}

template<typename T>
class VariableSizedGrid : public Grid<T>
{
public:
    VariableSizedGrid(int rows, int cols)
        : Grid<T>(rows, cols, new T[rows * cols])
    {
        
    }

    ~VariableSizedGrid()
    {
        delete [] _data;
    }
};