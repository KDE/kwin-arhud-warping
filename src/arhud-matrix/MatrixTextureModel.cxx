// SPDX-FileCopyrightText: Copyright (c) 2022 MBition GmbH.
// SPDX-License-Identifier: GPL-2.0-or-later

#include "MatrixTextureModel.hxx"

#include <algorithm>
#include <cmath>
#include <limits>

/**
 * @brief Constructs the MatrixTextureModel.
 */
MatrixTextureModel::MatrixTextureModel(uint32_t matrixCount, uint32_t x_dim, uint32_t y_dim)
    : mDimX(x_dim), mDimY(y_dim), mMatrices()
{
  mMatrices.reserve(matrixCount);
  for (uint32_t i = 0; i < matrixCount; i++)
  {
    mMatrices.emplace_back(x_dim, y_dim);
  }
}

/**
 * @brief Returns a reference to the matrix on the given index.
 *
 * @param[in] index The index of the matrix.
 *
 * @return The matrix on the given index. If the index is out of range, a reference to the matrix on index 0 is
 * returned.
 */
const Matrix& MatrixTextureModel::getMatrix(uint32_t index) const
{
  return mMatrices.at(index);
}

/**
 * @brief Sets the matrix data on a given index. If the index is out of range the function asserts.
 *
 * @param[in] index The index where the matrix has to be set.
 * @param[in] m The matrix to set.
 */
void MatrixTextureModel::setMatrix(uint32_t index, const Matrix& m)
{
  if (m.dimX() == mDimX && m.dimY() == mDimY)
  {
    mMatrices.at(index) = m;
  }
}

/**
 * @brief Sets all matrices.
 *
 * @param[in] matrices The matrix array.
 */
void MatrixTextureModel::setMatrices(const std::vector<Matrix>& matrices)
{
  mMatrices = matrices;
}

/**
 * @brief Returns the texture data so that all double values are in 4-byte integers encoded.
 *
 * @return The texture data in integers encoded.
 */
std::vector<uint8_t> MatrixTextureModel::getTextureData() const
{
  auto matrixCount        = mMatrices.size();
  auto matrixElementCount = mDimX * mDimY * 2;

  std::vector<uint8_t> bytes(matrixCount * matrixElementCount * 4);

  uint8_t* target = bytes.data();

  for (const Matrix& matrix : mMatrices)
  {
    const float64_t* source    = matrix.data();
    const float64_t* sourceEnd = source + matrixElementCount;

    while (source < sourceEnd)
    {
      float64_t value = *source;

      // We accept a displacement range in [-2.0, 2.0].
      float64_t d = std::clamp(value * 0.25 + 0.5, 0.0, 1.0);
      uint32_t  u = static_cast<uint32_t>(std::round(d * static_cast<float64_t>(std::numeric_limits<uint32_t>::max())));

      const uint32_t byteMask = 0xffU;

      target[0] = static_cast<uint8_t>(u & byteMask);
      target[1] = static_cast<uint8_t>((u >> 8) & byteMask);
      target[2] = static_cast<uint8_t>((u >> 16) & byteMask);
      target[3] = static_cast<uint8_t>((u >> 24) & byteMask);

      source += 1;
      target += 4;
    }
  }

  return bytes;
}
