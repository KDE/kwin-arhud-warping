/*
 * SPDX-FileCopyrightText: Copyright (c) 2017-2022, Daimler AG
 * SPDX-FileCopyrightText: Copyright (c) 2022, Mercedes-Benz AG
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

#include <cstdint>

namespace Warping
{
  /**
   * @brief Defines the display resolution width in pixels.
   */
  extern uint32_t DISPLAY_RESOLUTION_X;

  /**
   * @brief Defines the display resolution height in pixels.
   */
  extern uint32_t DISPLAY_RESOLUTION_Y;

  /*
   * @brief Defines the width (number of vertices in a row / x-direction) of the input warping matrix.
   */
  extern uint32_t WARPING_MATRIX_INPUT_RESOLUTION_X;

  /*
   * @brief Defines the height (number of vertices in a column / y-direction) of the input warping matrix.
   */
  extern uint32_t WARPING_MATRIX_INPUT_RESOLUTION_Y;

  /*
   * @brief Defines the number of warping matrices used for dynamic warping.
   */
  extern uint32_t WARPING_MATRIX_COUNT;

  /**
   * @brief Defines the content resolution width in pixels.
   */
  extern uint32_t CONTENT_RESOLUTION_X;

  /**
   * @brief Defines the content resolution height in pixels.
   */
  extern uint32_t CONTENT_RESOLUTION_Y;

  /**
   * @brief Defines the width (number of vertices in a row / x-direction) of the extrapolated warping matrix.
   */
  extern uint32_t WARPING_MATRIX_EXTRAPOLATED_RESOLUTION_X;

  /**
   * @brief Defines the height (number of vertices in a column / y-direction) of the extrapolated warping matrix.
   */
  extern uint32_t WARPING_MATRIX_EXTRAPOLATED_RESOLUTION_Y;
}
