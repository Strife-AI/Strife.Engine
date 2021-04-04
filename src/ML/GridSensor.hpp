#pragma once

#include "ML.hpp"
#include <torch/torch.h>

namespace StrifeML
{
    template<int Rows, int Cols>
    torch::Tensor PackIntoTensor(GridSensorOutput<Rows, Cols>& value)
    {
        FixedSizeGrid<uint64_t, Rows, Cols> grid;
        value.Decompress(grid);
        return PackIntoTensor((const Grid<uint64_t>&)grid);
    }

    template<int Rows, int Cols>
    struct TorchPacker<GridSensorOutput<Rows, Cols>, uint64_t>
    {
        static uint64_t* Pack(const GridSensorOutput<Rows, Cols>& value, uint64_t* outPtr)
        {
            if (value.IsCompressed())
            {
                FixedSizeGrid<uint64_t, Rows, Cols> decompressedGrid;
                value.Decompress(decompressedGrid);
                return TorchPacker<Grid<uint64_t>, uint64_t>::Pack(decompressedGrid, outPtr);
            }
            else
            {
                Grid<uint64_t> grid(Rows, Cols, const_cast<uint64_t*>(value.GetRawData()));
                return TorchPacker<Grid<uint64_t>, uint64_t>::Pack(grid, outPtr);
            }
        }
    };

    template<int Rows, int Cols>
    struct GetCellType<GridSensorOutput<Rows, Cols>>
    {
        using Type = uint64_t;
    };

    template<int Rows, int Cols>
    struct DimensionCalculator<GridSensorOutput<Rows, Cols>>
    {
        static constexpr auto Dims(const GridSensorOutput<Rows, Cols>& grid)
        {
            return Dimensions<2>(Rows, Cols).Union(DimensionCalculator<int>::Dims(0));
        }
    };
}