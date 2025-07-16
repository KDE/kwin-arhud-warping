/*
 * SPDX-FileCopyrightText: Copyright (c) 2017-2022, Daimler AG
 * SPDX-FileCopyrightText: Copyright (c) 2022, Mercedes-Benz AG
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

#include <array>
#include <vector>
#include <cstdint>
#include <limits>

#include "WarpingConstants.hxx"

typedef std::enable_if<std::numeric_limits<double>::is_iec559 && (sizeof(double) == sizeof(uint64_t)), double>::type
    float64_t;

namespace Warping
{

  class Matrix
  {
    public:
      Matrix(uint32_t x_dim, uint32_t y_dim)
        : mDimX(x_dim), mDimY(y_dim), mElements(x_dim * y_dim * 2) { }
      Matrix(uint32_t x_dim, uint32_t y_dim, float *values);

      float64_t get(uint32_t x, uint32_t y, uint32_t z) const;
      void set(uint32_t x, uint32_t y, uint32_t z, float64_t value);

      uint32_t dimX() const { return mDimX; }
      uint32_t dimY() const { return mDimY; }

      const float64_t* data() const { return mElements.data(); }

      void getExtendedWarpingMatrix(const std::array<float64_t, 2>& viewResolution, Matrix& em);

    private:
      uint32_t mDimX;
      uint32_t mDimY;
      std::vector<float64_t> mElements;

      void extrapolateLinear(Matrix& t);
  };

  //-------------------------------------------------------------------------------------

  std::array<float, 4> getUVFunc();
}  // namespace Warping
