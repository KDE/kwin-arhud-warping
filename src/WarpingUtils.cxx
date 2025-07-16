/*
 * SPDX-FileCopyrightText: Copyright (c) 2017-2022, Daimler AG
 * SPDX-FileCopyrightText: Copyright (c) 2022, Mercedes-Benz AG
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "WarpingUtils.hxx"

namespace Warping
{
  Matrix::Matrix(uint32_t x_dim, uint32_t y_dim, float *values)
    : mDimX(x_dim), mDimY(y_dim), mElements(x_dim * y_dim * 2)
  {
    uint32_t count = x_dim * y_dim * 2;

    for (uint32_t i = 0; i < count; i++)
    {
      mElements[i] = values[i];
    }
  }

  float64_t Matrix::get(uint32_t x, uint32_t y, uint32_t z) const
  {
    if (x < mDimX && y < mDimY && z < 2)
    {
      return mElements[(y * mDimX + x) * 2 + z];
    }

    return 0.0;
  }

  void Matrix::set(uint32_t x, uint32_t y, uint32_t z, float64_t value)
  {
    if (x < mDimX && y < mDimY && z < 2)
    {
      mElements[(y * mDimX + x) * 2 + z] = value;
    }
  }

  /**
   * @brief Returns the texture coordinate of a pixel indexed by pixelIndex according in display area space: the left
   * upper pixel CORNER has the coordinates (0, 0) and the right lower pixel the coordinates (1, 1).
   *
   * @param[in] displayAreaSize Display area size in pixels.
   * @param[in] pixelIndex Pixel index according to the same coordinate system.
   *
   * @return The pixel texture coordinate in display area space.
   */
  inline std::array<float64_t, 2> getPixelMiddleCoordinateInDisplayAreaSpace(
      const std::array<float64_t, 2>& displayAreaSize,
      const std::array<float64_t, 2>& pixelIndex)
  {
    return {{(static_cast<float64_t>(pixelIndex[0]) + 0.5) / static_cast<float64_t>(displayAreaSize[0]),
             (static_cast<float64_t>(pixelIndex[1]) + 0.5) / static_cast<float64_t>(displayAreaSize[1])}};
  }

  /**
   * @brief Returns the texture coordinate (UV) transformation parameters used by the warping. The following linear
   * transformation formula is used for the texture coordinates: texCoord = qt_MultiTexCoord0 * uvFunc.xy + uvFunc.zw;
   *
   * @return The texture coordinate (UV) transformation parameters
   */
  std::array<float, 4> getUVFunc()
  {
    std::array<float64_t, 2> displayAreaRes{{float64_t(DISPLAY_RESOLUTION_X), float64_t(DISPLAY_RESOLUTION_Y)}};
    std::array<float64_t, 2> contentAreaRes{{float64_t(CONTENT_RESOLUTION_X), float64_t(CONTENT_RESOLUTION_Y)}};
    std::array<float64_t, 2> gridCellSizeInPixels = {{contentAreaRes[0] / (WARPING_MATRIX_INPUT_RESOLUTION_X - 1),
                                                      contentAreaRes[1] / (WARPING_MATRIX_INPUT_RESOLUTION_Y - 1)}};
    const std::array<float64_t, 2> gridLeftUpperVertexPosInPixels = {
        {(displayAreaRes[0] - contentAreaRes[0]) / 2 - gridCellSizeInPixels[0],
         (displayAreaRes[1] - contentAreaRes[1]) / 2 - gridCellSizeInPixels[1]}};

    auto c0 = getPixelMiddleCoordinateInDisplayAreaSpace(displayAreaRes, gridLeftUpperVertexPosInPixels);
    auto c1 =
        getPixelMiddleCoordinateInDisplayAreaSpace(displayAreaRes,
                                                   {{gridLeftUpperVertexPosInPixels[0] + gridCellSizeInPixels[0],
                                                     gridLeftUpperVertexPosInPixels[1] + gridCellSizeInPixels[1]}});

    auto ax = (c1[0] - c0[0]) * (static_cast<float64_t>(WARPING_MATRIX_EXTRAPOLATED_RESOLUTION_X) - 1.0);
    auto ay = (c1[1] - c0[1]) * (static_cast<float64_t>(WARPING_MATRIX_EXTRAPOLATED_RESOLUTION_Y) - 1.0);

    return {{static_cast<float>(ax), static_cast<float>(ay), static_cast<float>(c0[0]), static_cast<float>(c0[1])}};
  }

  /**
   * @brief Returns the extended warping matrix for the parameter input warping matrix. The following operations are
   * performed:
   *          - Transformation from pixel middle indices to screen space coordinates in display area SCREEN space.
   *          - Transformation from display area SCREEN space to view screen space.
   *          - Linear matrix extrapolation.
   *
   * @param[in] wm The input warping matrix.
   * @param[in] viewResolution The resolution of the view.
   *
   * @return The extended warping matrix.
   */
  void Matrix::getExtendedWarpingMatrix(const std::array<float64_t, 2>& viewResolution, Matrix& em)
  {
    if (em.dimX() == WARPING_MATRIX_EXTRAPOLATED_RESOLUTION_X && em.dimY() == WARPING_MATRIX_EXTRAPOLATED_RESOLUTION_Y &&
        dimX() == WARPING_MATRIX_INPUT_RESOLUTION_X && dimY() == WARPING_MATRIX_INPUT_RESOLUTION_Y)
    {
      const float64_t w = static_cast<float64_t>(DISPLAY_RESOLUTION_X);
      const float64_t h = static_cast<float64_t>(DISPLAY_RESOLUTION_Y);

      const float64_t w1 = w - 1.0;
      const float64_t h1 = h - 1.0;

      const float64_t wr = w / static_cast<float64_t>(viewResolution[0]);
      const float64_t hr = h / static_cast<float64_t>(viewResolution[1]);

      // Coordinate transformations.
      Matrix m(WARPING_MATRIX_INPUT_RESOLUTION_X, WARPING_MATRIX_INPUT_RESOLUTION_Y);
      for (uint32_t y = 0; y < dimY(); y++)
      {
        for (uint32_t x = 0; x < dimX(); x++)
        {
          // Transformation from pixel middle indices to screen space coordinates in display area SCREEN space.
          float64_t ssPos0 = (get(x, y, 0) *  2.0 - w1) / w;
          float64_t ssPos1 = (get(x, y, 1) * -2.0 + h1) / h;

          // Transformation from display area SCREEN space to view screen space.
          m.set(x, y, 0, (ssPos0 + 1.0) * wr - 1.0);
          m.set(x, y, 1, (ssPos1 - 1.0) * hr + 1.0);
        }
      }

      m.extrapolateLinear(em);
      // wm.extrapolateLinear(em);  // output in pixel instead of normalized to [0..1]
    }
  }

  /**
   * @brief Extrapolates a matrix element-wise linearly. Note that for linear interpolation the result of
   * extrapolating borders in different order for the corner values DOES NOT result in different values, therefore the
   * implementation can freely choose an arbitrary order.
   *
   * @param[out] t The target (output, bigger, extrapolated) matrix.
   */
  void Matrix::extrapolateLinear(Matrix& t)
  {
    if (t.dimX() >= dimX() && t.dimY() >= dimY() &&
        ((t.dimX() - dimX()) % 2) == 0 && ((t.dimY() - dimY()) % 2) == 0)
    {
      const uint32_t xOffset = (t.dimX() - dimX()) / 2;
      const uint32_t yOffset = (t.dimY() - dimY()) / 2;

      // Copy inner part of the matrix.
      for (uint32_t y = 0; y < dimY(); y++)
      {
        for (uint32_t x = 0; x < dimX(); x++)
        {
          t.set(x + xOffset, y + yOffset, 0, get(x, y, 0));
          t.set(x + xOffset, y + yOffset, 1, get(x, y, 1));
        }
      }

      // Extrapolate in x-direction.
      const uint32_t yLimit   = dimY() + yOffset;
      const uint32_t sX1Start = dimX() + xOffset - 1;
      for (uint32_t y = yOffset; y < yLimit; y++)
      {
        for (uint32_t i = 0; i < xOffset; i++)
        {
          const uint32_t sX0 = xOffset - i;
          const uint32_t sX1 = sX1Start + i;
          for (uint32_t c = 0; c < 2; c++)
          {
            t.set(sX0 - 1, y, c, t.get(sX0, y, c) * 2.0 - t.get(sX0 + 1, y, c));
            t.set(sX1 + 1, y, c, t.get(sX1, y, c) * 2.0 - t.get(sX1 - 1, y, c));
          }
        }
      }

      // Extrapolate in y-direction.
      const uint32_t sY1Start = dimY() + yOffset - 1;
      for (uint32_t i = 0; i < yOffset; i++)
      {
        for (uint32_t x = 0; x < t.dimX(); x++)
        {
          const uint32_t sY0 = yOffset - i;
          const uint32_t sY1 = sY1Start + i;
          for (uint32_t c = 0; c < 2; c++)
          {
            t.set(x, sY0 - 1, c, t.get(x, sY0, c) * 2.0 - t.get(x, sY0 + 1, c));
            t.set(x, sY1 + 1, c, t.get(x, sY1, c) * 2.0 - t.get(x, sY1 - 1, c));
          }
        }
      }
    }
  }

}  // namespace Warping
