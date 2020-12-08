#pragma once

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
        for(int i = 0; i < TotalDimensions; ++i) result.dimensions[i] = dimensions[i];
        for(int i = 0; i < RhsTotalDimensions; ++i) result.dimensions[i + TotalDimensions] = rhs.dimensions[i];
        return result;
    }

    static int GetTotalDimensions()
    {
        return TotalDimensions;
    }

    int64_t dimensions[TotalDimensions] { 0 };
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

template<int Rows, int Cols>
struct DimensionCalculator<GridSensorOutput<Rows, Cols>>
{
    static constexpr auto Dims(const GridSensorOutput<Rows, Cols>& grid)
    {
        return Dimensions<2>(Rows, Cols).Union(DimensionCalculator<int>::Dims(0));
    }
};

template<typename T>
struct DimensionCalculator<gsl::span<T>>
{
    static constexpr auto Dims(const gsl::span<T>& span)
    {
        return Dimensions<1>(span.size()).Union(DimensionCalculator<T>::Dims(span[0]));
    }
};

template<typename ...TArgs>
struct DimensionCalculator<std::tuple<TArgs...>>
{
// TODO: implement for tuple
};

template<typename T, typename Enable = void> struct GetCellType;
template<typename T> struct GetCellType<T, std::enable_if<std::is_arithmetic_v<T>>> { using Type = T; };
template<typename TCell> struct GetCellType<Grid<TCell>> { using Type = typename GetCellType<TCell>::Type; };
template<int Rows, int Cols> struct GetCellType<GridSensorOutput<Rows, Cols>> { using Type = int; };
template<typename T> struct GetCellType<gsl::span<T>> { using Type = typename GetCellType<T>::Type; };

template<typename T>
c10::ScalarType GetTorchType();

template<> c10::ScalarType GetTorchType<int>() { return torch::kInt32; }
template<> c10::ScalarType GetTorchType<float>() { return torch::kFloat32; }
template<> c10::ScalarType GetTorchType<uint64_t>() { return torch::kInt64; }
template<> c10::ScalarType GetTorchType<double>() { return torch::kFloat64; }

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

template<int Rows, int Cols>
struct TorchPacker<GridSensorOutput<Rows, Cols>, int>
{
static int* Pack(const GridSensorOutput<Rows, Cols>& value, int* outPtr)
{
    if(value.IsCompressed())
    {
        FixedSizeGrid<int, Rows, Cols> decompressedGrid;
        value.Decompress(decompressedGrid);
        return TorchPacker<Grid<int>, int>::Pack(decompressedGrid, outPtr);
    }
    else
    {
        Grid<int> grid(Rows, Cols, const_cast<int*>(value.GetRawData()));
        return TorchPacker<Grid<int>, int>::Pack(grid, outPtr);
    }
}
};

template<typename T>
torch::Tensor PackIntoTensor(const T& value)
{
    using CellType = typename GetCellType<T>::Type;
    auto dimensions = DimensionCalculator<T>::Dims(value);

    torch::IntArrayRef dims(dimensions.dimensions, dimensions.GetTotalDimensions());
    auto torchType = GetTorchType<CellType>();
    auto t = torch::empty(dims, torchType);

    TorchPacker<T, CellType>::Pack(value, t.template data_ptr<CellType>());

    return t.squeeze();
}

template<int Rows, int Cols>
torch::Tensor PackIntoTensor(GridSensorOutput<Rows, Cols>& value)
{
    FixedSizeGrid<int, Rows, Cols> grid;
    value.Decompress(grid);
    return PackIntoTensor((const Grid<int>&)grid);
}

template<typename T, typename TSelector>
torch::Tensor PackIntoTensor(const Grid<T>& grid, TSelector selector)
{
    using SelectorReturnType = decltype(selector(grid[0][0]));
    using CellType = typename GetCellType<SelectorReturnType>::Type;

    SelectorReturnType selectorTemp;
    Grid<SelectorReturnType> dummyGrid(grid.Rows(), grid.Cols(), &selectorTemp);

    auto dimensions = DimensionCalculator<Grid<SelectorReturnType>>::Dims(dummyGrid);

    torch::IntArrayRef dims(dimensions.dimensions, dimensions.GetTotalDimensions());
    auto torchType = GetTorchType<CellType>();
    auto t = torch::empty(dims, torchType);

    auto outPtr = t.template data_ptr<CellType>();

    for(int i = 0; i < grid.Rows(); ++i)
    {
        for(int j = 0; j < grid.Cols(); ++j)
        {
            auto selectedValue = selector(grid[i][j]);
            outPtr = TorchPacker<SelectorReturnType, CellType>::Pack(selectedValue, outPtr);
        }
    }

    return t.squeeze();
}

template<typename T, typename TSelector>
torch::Tensor PackIntoTensor(const gsl::span<T>& span, TSelector selector)
{
    // Just treat the span as a grid of 1 x span.size() since the dimensions get squeezed anyway
    Grid<const T> grid(1, span.size(), span.data());
    return PackIntoTensor(grid, selector);
}