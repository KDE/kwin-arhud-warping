// SPDX-FileCopyrightText: Copyright (c) 2022 MBition GmbH.
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "MatrixTextureModel.hxx"
#include "WarpingMatrixInterpolationModel.hxx"
#include "WarpingUtils.hxx"

#include <effect/effect.h>

#include <memory>

class MBitionWarpedOutput;
class MBitionWarpedOutputManager;

namespace KWaylandServer
{
  class OutputInterface;
}

namespace KWin
{
  class GLFramebuffer;
  class GLShader;
  class GLTexture;

  class ArHUDEffect : public Effect
  {
    Q_OBJECT

  public:
    ArHUDEffect();
    ~ArHUDEffect() override;

    void paintScreen(const RenderTarget &renderTarget, const RenderViewport &viewport, int mask, const QRegion &region, Output *screen) override;

    bool isActive() const override;

    int requestedEffectChainPosition() const override
    {
      return 99;
    }
    static bool supported();

    void checkGlTexture(Output* screen);
    MBitionWarpedOutput* warpedOutput(Output* screen);

  private:
    uint32_t m_vertexCount = 0;

    std::unique_ptr<GLTexture>                  m_GLtexture;
    std::unique_ptr<GLFramebuffer>              m_GLframebuffer;
    std::unique_ptr<MBitionWarpedOutput>        m_warpedOutput;
    std::unique_ptr<MBitionWarpedOutputManager> m_warpedOutputManager;

    Output* m_warpedScreen = nullptr;

    void addScreen(Output* screen);
    void removeScreen(Output* screen);

    std::unique_ptr<GLShader> m_shader;

    int m_modelViewProjectioMatrixLocation  = -1;
    int m_warpingMatrixTextureLocation      = -1;
    int m_matrixCountLocation               = -1;
    int m_matrixResolutionLocation          = -1;
    int m_matrixInterpolationFactorLocation = -1;
    int m_matrixInterpolationIndexLocation  = -1;
    int m_inputTextureLocation              = -1;
    int m_uvFunctLocation                   = -1;
  };

}  // namespace KWin
