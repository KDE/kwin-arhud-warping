// SPDX-FileCopyrightText: Copyright (c) 2022 MBition GmbH.
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "WarpingUtils.hxx"

#include <cstddef>
#include <cstdint>

using namespace Warping;

/**
 * @brief Stores the warping matrices in floating-point representation and implements the conversion to matrix texture
 * byte data form.
 */
class MatrixTextureModel final
{
public:
  MatrixTextureModel(uint32_t matrixCount, uint32_t x_dim, uint32_t y_dim);
  const Matrix&         getMatrix(uint32_t index) const;
  void                  setMatrix(uint32_t index, const Matrix& m);
  void                  setMatrices(const std::vector<Matrix>& matrices);
  std::vector<uint8_t>  getTextureData() const;

  /**
   * @brief Stores array of matrices.
   */
  uint32_t mDimX;
  uint32_t mDimY;
  std::vector<Matrix> mMatrices;
};
