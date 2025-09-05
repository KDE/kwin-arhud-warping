// SPDX-FileCopyrightText: Copyright (c) 2025 MBition GmbH.
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <effect/effect.h>
#include <vector>

struct ShaderRegion;
class MBitionMiniHudWarping;
class MBitionMiniHudWarpingManager;

using params_t = std::array<float, 378>;

namespace KWin
{

class GLTexture;
class GLFramebuffer;
class GLShader;
class GLVertexBuffer;

class WarpingEffect;

class DefaultHudEffect : public QObject
{
    Q_OBJECT

public:
    DefaultHudEffect();
    ~DefaultHudEffect();

    void paintScreen(const RenderTarget& renderTarget, const RenderViewport& viewport, int mask, const QRegion& region, Output* screen);
    bool isActive() const;

    void checkGlTexture();
    MBitionMiniHudWarping* miniHud(Output* screen);

    void setHudSize(unsigned int displayWidth, unsigned int displayHeight, unsigned int appAreaWidth, unsigned int appAreaHeight);

    void setMatrices(int fd);
    void setMirrorLevel(float mirrorLevel);
    void setWhitePoint(float red, float green, float blue);

private:
    void setupShaderRegions(const params_t& params1, const params_t& params2, const params_t& params3);

    struct {
        unsigned int displayWidth{ 0 };
        unsigned int displayHeight{ 0 };
        unsigned int appAreaWidth{ 0 };
        unsigned int appAreaHeight{ 0 };
    } m_hudSize;
    uint32_t m_vertexDimensions;
    QVector3D m_whitePoint;
    float m_mirrorLevel;

    Output* m_screen;
    std::unique_ptr<GLShader> m_shader;
    std::unique_ptr<GLTexture> m_texture;
    std::unique_ptr<GLFramebuffer> m_framebuffer;
    std::unique_ptr<MBitionMiniHudWarping> m_miniHud;
    std::unique_ptr<MBitionMiniHudWarpingManager> m_miniHudManager;
    std::vector<ShaderRegion> m_shaderRegions;

    int m_params1_x_location = -1;
    int m_params1_y_location = -1;
    int m_params2_x_location = -1;
    int m_params2_y_location = -1;
    int m_params3_x_location = -1;
    int m_params3_y_location = -1;
    int m_mirrorLevel_location = -1;
    int m_whitePointCorrection_location = -1;
    int m_source_location = -1;
    int m_uv_span_location = -1;
    int m_window_size_location = -1;
};

} // namespace KWin
