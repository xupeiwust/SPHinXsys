/**
 * @file 	mesh_iterators.hpp
 * @brief 	Here, Functions belong to mesh iterators.
 * @author	hi Zhang and Xiangyu Hu
 */

#pragma once

#include "mesh_iterators.h"

namespace SPH
{
//=================================================================================================//
template <int lower0, int upper0,
          int lower1, int upper1,
          int lower2, int upper2, typename FunctionOnEach>
inline void mesh_for_each3d(const FunctionOnEach &function)
{
    for (int l = lower0; l != upper0; ++l)
        for (int m = lower1; m != upper1; ++m)
            for (int n = lower2; n != upper2; ++n)
            {
                function(l, m, n);
            }
}
//=================================================================================================//
template <int lower0, int upper0,
          int lower1, int upper1,
          int lower2, int upper2, typename CheckOnEach>
inline Array3i mesh_find_if3d(const CheckOnEach &function)
{
    for (int l = lower0; l != upper0; ++l)
        for (int m = lower1; m != upper1; ++m)
            for (int n = lower2; n != upper2; ++n)
            {
                if (function(l, m, n))
                    return Array3i(l, m, n);
            }
    return Array3i(upper0, upper1, upper2);
}
//=================================================================================================//
template <typename FunctionOnEach>
void mesh_for_each(const Array3i &lower, const Array3i &upper, const FunctionOnEach &function)
{
    for (int l = lower[0]; l != upper[0]; ++l)
        for (int m = lower[1]; m != upper[1]; ++m)
            for (int n = lower[2]; n != upper[2]; ++n)
            {
                function(Array3i(l, m, n));
            }
}
//=================================================================================================//
template <typename FunctionOnEach>
Array3i mesh_find_if(const Array3i &lower, const Array3i &upper, const FunctionOnEach &function)
{
    for (int l = lower[0]; l != upper[0]; ++l)
        for (int m = lower[1]; m != upper[1]; ++m)
            for (int n = lower[2]; n != upper[2]; ++n)
            {
                if (function(l, m, n))
                    return Array3i(l, m, n);
            }
    return upper;
}
//=================================================================================================//
//=================================================================================================//
template <typename LocalFunction, typename... Args>
void mesh_for(const MeshRange &mesh_range, const LocalFunction &local_function, Args &&...args)
{
    for (int i = (mesh_range.first)[0]; i != (mesh_range.second)[0]; ++i)
        for (int j = (mesh_range.first)[1]; j != (mesh_range.second)[1]; ++j)
            for (int k = (mesh_range.first)[2]; k != (mesh_range.second)[2]; ++k)
            {
                local_function(Array3i(i, j, k));
            }
}
//=================================================================================================//
template <typename LocalFunction, typename... Args>
void mesh_parallel_for(const MeshRange &mesh_range, const LocalFunction &local_function, Args &&...args)
{
    parallel_for(
        IndexRange3d((mesh_range.first)[0], (mesh_range.second)[0],
                     (mesh_range.first)[1], (mesh_range.second)[1],
                     (mesh_range.first)[2], (mesh_range.second)[2]),
        [&](const IndexRange3d &r)
        {
            for (size_t i = r.pages().begin(); i != r.pages().end(); ++i)
                for (size_t j = r.rows().begin(); j != r.rows().end(); ++j)
                    for (size_t k = r.cols().begin(); k != r.cols().end(); ++k)
                    {
                        local_function(Array3i(i, j, k));
                    }
        },
        ap);
}
//=================================================================================================//
template <typename LocalFunction, typename... Args>
void mesh_stride_forward_for(const MeshRange &mesh_range, const Arrayi &stride, const LocalFunction &local_function, Args &&...args)
{
    for (int m = 0; m < stride[0]; m++)
        for (int n = 0; n < stride[1]; n++)
            for (int p = 0; p < stride[2]; p++)
                for (auto i = (mesh_range.first)[0] + m; i < (mesh_range.second)[0]; i += stride[0])
                    for (auto j = (mesh_range.first)[1] + n; j < (mesh_range.second)[1]; j += stride[1])
                        for (auto k = (mesh_range.first)[2] + p; k < (mesh_range.second)[2]; k += stride[2])
                        {
                            local_function(Array3i(i, j, k));
                        }
}
//=================================================================================================//
template <typename LocalFunction, typename... Args>
void mesh_stride_forward_parallel_for(const MeshRange &mesh_range, const Arrayi &stride, const LocalFunction &local_function, Args &&...args)
{
    for (int m = 0; m < stride[0]; m++)
        for (int n = 0; n < stride[1]; n++)
            for (int p = 0; p < stride[2]; p++)
            {
                parallel_for((mesh_range.first)[0] + m, (mesh_range.second)[0], stride[0], [&](auto i)
                             { parallel_for((mesh_range.first)[1] + n, (mesh_range.second)[1], stride[1], [&](auto j)
                                            { parallel_for((mesh_range.first)[2] + p, (mesh_range.second)[2], stride[2], [&](auto k)
                                                           { local_function(Array3i(i, j, k)); },
                                                           ap); }, ap); }, ap);
            }
}
//=================================================================================================//
template <typename LocalFunction, typename... Args>
void mesh_stride_backward_for(const MeshRange &mesh_range, const Arrayi &stride, const LocalFunction &local_function, Args &&...args)
{
    for (int m = stride[0]; m > 0; m--)
        for (int n = stride[1]; n > 0; n--)
            for (int p = stride[2]; p > 0; p--)
                for (auto i = (mesh_range.first)[0] + m - 1; i < (mesh_range.second)[0]; i += stride[0])
                    for (auto j = (mesh_range.first)[1] + n - 1; j < (mesh_range.second)[1]; j += stride[1])
                        for (auto k = (mesh_range.first)[2] + p - 1; k < (mesh_range.second)[2]; k += stride[2])
                        {
                            local_function(Array3i(i, j, k));
                        }
}
//=================================================================================================//
template <typename LocalFunction, typename... Args>
void mesh_stride_backward_parallel_for(const MeshRange &mesh_range, const Arrayi &stride, const LocalFunction &local_function, Args &&...args)
{
    for (int m = stride[0]; m > 0; m--)
        for (int n = stride[1]; n > 0; n--)
            for (int p = stride[2]; p > 0; p--)
            {
                parallel_for((mesh_range.first)[0] + m - 1, (mesh_range.second)[0], stride[0], [&](auto i)
                             { parallel_for((mesh_range.first)[1] + n - 1, (mesh_range.second)[1], stride[1], [&](auto j)
                                            { parallel_for((mesh_range.first)[2] + p - 1, (mesh_range.second)[2], stride[2], [&](auto k)
                                                           { local_function(Array3i(i, j, k)); },
                                                           ap); }, ap); }, ap);
            }
}
//=================================================================================================//
} // namespace SPH
