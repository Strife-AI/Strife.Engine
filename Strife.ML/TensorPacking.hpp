#pragma once

#include <array>
#include <torch/torch.h>

#if STRIFE_ENGINE
    #include "ML/ML.hpp"
#endif

namespace StrifeML
{
    template<int TotalDimensions>
    struct Dimensions
    {
        template<typename ...Args>
        constexpr Dimensions(Args&& ...dims)
            : dimensions{dims...}
        {

        }

        template<int RhsTotalDimensions>
        constexpr Dimensions<TotalDimensions + RhsTotalDimensions> Union(const Dimensions<RhsTotalDimensions>& rhs) const
        {
            Dimensions<TotalDimensions + RhsTotalDimensions> result;
            for (int i = 0; i < TotalDimensions; ++i) result.dimensions[i] = dimensions[i];
            for (int i = 0; i < RhsTotalDimensions; ++i) result.dimensions[i + TotalDimensions] = rhs.dimensions[i];
            return result;
        }

        static int GetTotalDimensions()
        {
            return TotalDimensions;
        }

        int64_t dimensions[TotalDimensions]{0};
    };

    template<typename T, typename Enable = void>
    struct DimensionCalculator
    {

    };

    template<typename T>
    struct DimensionCalculator<T, std::enable_if_t<std::is_arithmetic_v<T>>>
    {
        static constexpr auto Dims(const T& value)
        {
            return Dimensions<1>(1);
        }
    };

    template<typename TCell>
    struct DimensionCalculator<Grid<TCell>>
    {
        static constexpr auto Dims(const Grid<TCell>& grid)
        {
            return Dimensions<2>(grid.Rows(), grid.Cols()).Union(DimensionCalculator<TCell>::Dims(grid[0][0]));
        }
    };

#if STRIFE_ENGINE
    template<int Rows, int Cols>
    struct DimensionCalculator<GridSensorOutput<Rows, Cols>>
    {
        static constexpr auto Dims(const GridSensorOutput<Rows, Cols>& grid)
        {
            return Dimensions<2>(Rows, Cols).Union(DimensionCalculator<int>::Dims(0));
        }
    };
#endif

    template<typename T, std::size_t Size>
    struct DimensionCalculator<std::array<T, Size>>
    {
        static constexpr auto Dims(const std::array<T, Size>& arr)
        {
            return Dimensions<1>((int64_t)Size).Union(DimensionCalculator<T>::Dims(arr[0]));
        }
    };

    template<typename T>
    struct DimensionCalculator<gsl::span<T>>
    {
        static constexpr auto Dims(const gsl::span<T>& span)
        {
            return Dimensions<1>((long long)span.size()).Union(DimensionCalculator<T>::Dims(span[0]));
        }

    };

    template<typename T, typename Enable = void>
    struct GetCellType;

    template<typename T>
    struct GetCellType<T, typename std::enable_if<std::is_arithmetic<T>::value>::type>
    {
        using Type = T;
    };
    template<typename TCell>
    struct GetCellType<Grid<TCell>>
    {
        using Type = typename GetCellType<TCell>::Type;
    };

#if STRIFE_ENGINE
    template<int Rows, int Cols>
    struct GetCellType<GridSensorOutput<Rows, Cols>>
    {
        using Type = uint64_t;
    };
#endif

    template<typename T, std::size_t Size>
    struct GetCellType<std::array<T, Size>>
    {
        using Type = T;
    };

    template<typename T>
    struct GetCellType<gsl::span<T>>
    {
        using Type = typename GetCellType<T>::Type;
    };

    template<typename T>
    inline c10::ScalarType GetTorchType();

    template<>
    inline c10::ScalarType GetTorchType<int>()
    { return torch::kInt32; }

    template<>
    inline c10::ScalarType GetTorchType<float>()
    { return torch::kFloat32; }

    template<>
    inline c10::ScalarType GetTorchType<uint64_t>()
    { return torch::kInt64; }

    template<>
    inline c10::ScalarType GetTorchType<int64_t>()
    { return torch::kInt64; }

    template<>
    inline c10::ScalarType GetTorchType<double>()
    { return torch::kFloat64; }

    template<typename T, typename TorchType = T>
    struct TorchPacker
    {

    };

    template<>
    struct TorchPacker<int>
    {
        static int* Pack(const int& value, int* outPtr)
        {
            *outPtr = value;
            return outPtr + 1;
        }
    };

    template<>
    struct TorchPacker<int64_t>
    {
        static int64_t* Pack(const int64_t& value, int64_t* outPtr)
        {
            *outPtr = value;
            return outPtr + 1;
        }
    };

    template<>
    struct TorchPacker<float_t>
    {
        static float_t* Pack(const float_t& value, float_t* outPtr)
        {
            *outPtr = value;
            return outPtr + 1;
        }
    };

    template<typename TCell, typename TorchType>
    struct TorchPacker<gsl::span<TCell>, TorchType>
    {
        static TorchType* Pack(const gsl::span<TCell>& value, TorchType* outPtr)
        {
            if constexpr (std::is_arithmetic_v<TCell>)
            {
                memcpy(outPtr, &value[0], value.size() * sizeof(TCell));
                return outPtr + value.size();
            }
            else
            {
                for (int i = 0; i < (int)value.size(); ++i)
                {
                    outPtr = TorchPacker<TCell, TorchType>::Pack(value[i], outPtr);
                }

                return outPtr;
            }
        }
    };
	
    template<typename TCell, typename TorchType>
    struct TorchPacker<Grid<TCell>, TorchType>
    {
        static TorchType* Pack(const Grid<TCell>& value, TorchType* outPtr)
        {
            if constexpr (std::is_arithmetic_v<TCell>)
            {
                memcpy(outPtr, &value[0][0], value.Rows() * value.Cols() * sizeof(TCell));
                return outPtr + value.Rows() * value.Cols();
            }
            else
            {
                for (int i = 0; i < value.Rows(); ++i)
                {
                    for (int j = 0; j < value.Cols(); ++j)
                    {
                        outPtr = TorchPacker<TCell, TorchType>::Pack(value[i][j], outPtr);
                    }
                }

                return outPtr;
            }
        }
    };

    template<typename T, std::size_t  Size, typename TorchType>
    struct TorchPacker<std::array<T, Size>, TorchType>
    {
        static TorchType* Pack(const std::array<T, Size>& value, TorchType* outPtr)
        {
            if constexpr (std::is_arithmetic_v<T>)
            {
                memcpy(outPtr, &value[0], Size * sizeof(T));
                return outPtr + Size;
            }
            else
            {
                for (int i = 0; i < Size; ++i)
                {
                    outPtr = TorchPacker<T, TorchType>::Pack(value[i], outPtr);
                }

                return outPtr;
            }
        }
    };

#if STRIFE_ENGINE
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
#endif

    template<typename T>
    torch::Tensor PackIntoTensor(const T& value)
    {
        using CellType = typename GetCellType<T>::Type;
        auto dimensions = DimensionCalculator<T>::Dims(value);

        torch::IntArrayRef dims(dimensions.dimensions, dimensions.GetTotalDimensions());
        auto torchType = GetTorchType<CellType>();
        auto t = torch::empty(dims, torchType);

        TorchPacker<T, CellType>::Pack(value, t.template data_ptr<CellType>());

        return t.squeeze(t.dim()-1);
    }

#if STRIFE_ENGINE
    template<int Rows, int Cols>
    torch::Tensor PackIntoTensor(GridSensorOutput<Rows, Cols>& value)
    {
        FixedSizeGrid<uint64_t, Rows, Cols> grid;
        value.Decompress(grid);
        return PackIntoTensor((const Grid<uint64_t>&) grid);
    }
#endif

    template<typename T, typename TSelector>
    torch::Tensor PackIntoTensor(const Grid<T>& grid, TSelector selector)
    {
        using SelectorReturnType = decltype(selector(grid[0][0]));
        using CellType = typename GetCellType<SelectorReturnType>::Type;

        SelectorReturnType selectorTemp = selector(grid[0][0]);
        Grid<SelectorReturnType> dummyGrid(grid.Rows(), grid.Cols(), &selectorTemp);

        auto dimensions = DimensionCalculator<Grid<SelectorReturnType>>::Dims(dummyGrid);

        torch::IntArrayRef dims(dimensions.dimensions, dimensions.GetTotalDimensions());
        auto torchType = GetTorchType<CellType>();
        auto t = torch::empty(dims, torchType);

        CellType* outPtr;
        if constexpr (std::is_integral_v<CellType>)
        {
	        outPtr = reinterpret_cast<CellType*>(t.template data_ptr<std::make_signed_t<CellType>>());
        }
        else
        {
	        outPtr = t.template data_ptr<CellType>();
        }

        for (int i = 0; i < grid.Rows(); ++i)
        {
            for (int j = 0; j < grid.Cols(); ++j)
            {
                auto selectedValue = selector(grid[i][j]);
                outPtr = TorchPacker<SelectorReturnType, CellType>::Pack(selectedValue, outPtr);
            }
        }

        return t.squeeze(t.dim()-1);
    }

    template<typename T, typename TSelector>
    torch::Tensor PackIntoTensor(const gsl::span<T>& span, TSelector selector)
    {
        // Just treat the span as a grid of 1 x span.size() since the dimensions get squeezed anyway
        Grid<const T> grid(1, span.size(), span.data());
        return PackIntoTensor(grid, selector);
    }
}
