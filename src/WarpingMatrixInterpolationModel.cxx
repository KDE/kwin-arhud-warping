/*
 * SPDX-FileCopyrightText: Copyright (c) 2017-2022, Daimler AG
 * SPDX-FileCopyrightText: Copyright (c) 2022, Mercedes-Benz AG
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

/**
 * @file WarpingMatrixInterpolationModel.cxx
 *
 * @brief Definition of the class WarpingMatrixInterpolationModel.
 *
 * @author szeptam
 * @date 2017-07-17
 */
#include "WarpingMatrixInterpolationModel.hxx"

#include <type_traits>

// Z-Coordinate for interpolation
constexpr auto INTERPOLATION_COORDINATE = 2;

/**
 * @brief Constructs the object.
 *
 * @param[in] defaultPosition The default position in ego vehicle coordinate system and meters.
 */
WarpingMatrixInterpolationModel::WarpingMatrixInterpolationModel(uint32_t matrixCount)
  : mReferenceEyePositions(matrixCount)
{
  // TODO set default eye position to reference position of milddle matrix
  mEyePosition.fill(0);
}

/**
 * @brief Gets the interpolation parameters needed by the view.
 * The interpolation formula is matrix[index] * (1 - factor) + matrix[index + 1] * factor.
 *
 * param[out] index  The interpolation index.
 * param[out] factor The interpolation factor.
 */
void WarpingMatrixInterpolationModel::getInterpolationParameters(int32_t& index, float& factor) const
{
  const std::size_t count = Warping::WARPING_MATRIX_COUNT;
  // Making sure, that we don't access a non-existing matrix (-1 or n) in the shader code.
  const float64_t p = mEyePosition[INTERPOLATION_COORDINATE];
  if (p >= mReferenceEyePositions[0][INTERPOLATION_COORDINATE])
  {
    index  = 0;
    factor = 0.0f;
  }
  else if (p <= mReferenceEyePositions[count - 1][INTERPOLATION_COORDINATE])
  {
    index  = count - 2;
    factor = 1.0f;
  }
  else
  {
    for (std::size_t i = 1; i < count; i++)
    {
      if (p >= mReferenceEyePositions[i][INTERPOLATION_COORDINATE])
      {
        index             = static_cast<int32_t>(i - 1);
        const float64_t h = mReferenceEyePositions[static_cast<std::size_t>(index)][INTERPOLATION_COORDINATE];
        const float64_t l = mReferenceEyePositions[i][INTERPOLATION_COORDINATE];
        factor            = static_cast<float>((h - p) / (h - l));

        break;
      }
    }
  }
}

/**
 * @brief Sets the eye position and BREAKS the eye position BINDING.
 *
 * param[in] eyePosition The eye position to set.
 */
void WarpingMatrixInterpolationModel::setEyePosition(const Position& eyePosition)
{
  mEyePosition = eyePosition;
}

/**
 * @brief Sets the reference eye position.
 *
 * param[in] index                The index of the reference eye position.
 * param[in] referenceEyePosition The position for the reference eye position to set.
 *
 * @return Whether the parameter index and reference eye positions were valid.
 */
bool WarpingMatrixInterpolationModel::setReferenceEyePosition(const uint32_t  index,
                                                              const Position& referenceEyePosition)
{
  mReferenceEyePositions[index] = referenceEyePosition;
  return true;
}
