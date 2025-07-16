/*
 * SPDX-FileCopyrightText: Copyright (c) 2017-2022, Daimler AG
 * SPDX-FileCopyrightText: Copyright (c) 2022, Mercedes-Benz AG
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

#include "WarpingUtils.hxx"
#include <array>
#include <cstdint>

/**
 * @brief Eye position interpolation used for dynamic warping.
 */
class WarpingMatrixInterpolationModel final
{
public:
  using Position = std::array<float64_t, 3>;

  explicit WarpingMatrixInterpolationModel(uint32_t matrixCount);
  ~WarpingMatrixInterpolationModel() = default;

  void getInterpolationParameters(int32_t& index, float& factor) const;

  bool setReferenceEyePosition(const uint32_t index, const Position& referenceEyePosition);
  void setEyePosition(const Position& eyePosition);

private:
  WarpingMatrixInterpolationModel(const WarpingMatrixInterpolationModel& other)            = delete;
  WarpingMatrixInterpolationModel& operator=(const WarpingMatrixInterpolationModel& other) = delete;

  /**
   * @brief Middle eye position in vehicle coordinate system (in m).
   */
  Position mEyePosition;

  /**
   * @brief Reference middle eye positions in vehicle coordinate system (in m).
   */
  std::vector<Position> mReferenceEyePositions;
};
