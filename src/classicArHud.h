// SPDX-FileCopyrightText: Copyright (c) 2022-2025 MBition GmbH.
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <effect/effect.h>

#include "MatrixTextureModel.hxx"
#include "WarpingMatrixInterpolationModel.hxx"
#include "WarpingUtils.hxx"

#include <memory>

class MBitionWarpedOutput;
class MBitionWarpedOutputManager;

namespace KWin
{

class GLFramebuffer;
class GLShader;
class GLTexture;

class WarpingEffect;

class ClassicArHudEffect : public QObject
{
    Q_OBJECT

public:
    ClassicArHudEffect();
    ~ClassicArHudEffect();

    /**
     * @brief paint something on top of the windows. (one or multiple drawing and effects).
     * @param[in] mask - A set of flags that control or specify various options or conditions for painting the screen.
     * There are some enums for this purpose in kwineffects.h This function is called by wayland in every render loop.
     * @param[in] region - Specifying areas of a graphical user interface that need to be painted or updated.
     * @param[in] data - A set of data for painting the window/effects on the screen(tranlation, rotation, scale, ...).
     */
    void paintScreen(const RenderTarget &renderTarget, const RenderViewport &viewport, int mask, const QRegion &region, Output *screen);
    bool isActive() const;

    void checkGlTexture(Output* screen);
    MBitionWarpedOutput* warpedOutput(Output* screen);

private:
    uint32_t m_vertexCount = 0;

    std::unique_ptr<GLTexture>                  m_GLtexture;
    std::unique_ptr<GLFramebuffer>              m_GLframebuffer;
    std::unique_ptr<MBitionWarpedOutput>        m_warpedOutput;
    std::unique_ptr<MBitionWarpedOutputManager> m_warpedOutputManager;

    Output* m_warpedScreen = nullptr;
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
