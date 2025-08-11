/*
 * SPDX-FileCopyrightText: Copyright (c) 2017-2022, Daimler AG
 * SPDX-FileCopyrightText: Copyright (c) 2022, Mercedes-Benz AG
 * SPDX-FileCopyrightText: Copyright (c) 2022-2025 MBition GmbH.
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "classicArHud.h"
#include <QString>
#include <core/output.h>
#include <core/rendertarget.h>
#include <core/renderviewport.h>
#include <effect/effecthandler.h>
#include <opengl/glframebuffer.h>
#include <opengl/glshader.h>
#include <opengl/glshadermanager.h>
#include <opengl/gltexture.h>
#include <opengl/glvertexbuffer.h>
#include <wayland/display.h>
#include <wayland/output.h>

#include "warpingEffect.h"
#include "MBitionWarpedOutput.h"
#include "MBitionWarpedOutputManager.h"

#include <memory>
#include <QFile>
#include <QJsonDocument>
#include "kwinarhud_debug.h"

namespace KWin
{

// TODO Make sure that wayland callbacks and paintScreen get called from the same threads
ClassicArHudEffect::ClassicArHudEffect()
    : m_shader(
        ShaderManager::instance()->generateShaderFromFile(ShaderTrait::MapTexture,
                                                          QStringLiteral(":/effects/arhud/shaders/warping_arhud_classic.vert"),
                                                          QStringLiteral(":/effects/arhud/shaders/warping_arhud_classic.frag")))
{
    if (!m_shader->isValid())
    {
        qCWarning(KWINARHUD_DEBUG) << "Shader is not valid!";
        return;
    }

    qCInfo(KWINARHUD_DEBUG) << "Loading ClassicArHudEffect";

    QFile f(QStringLiteral("/opt/ui/kde/config/WarpingConstants.json"));

    if (f.open(QIODevice::ReadOnly))
    {
        QJsonParseError parseError;
        QJsonDocument doc = QJsonDocument::fromJson(f.readAll(), &parseError);

        if (!doc.isNull())
        {
            auto obj = doc.object();

            DISPLAY_RESOLUTION_X = obj[u"DISPLAY_RESOLUTION_X"].toInt();
            DISPLAY_RESOLUTION_Y = obj[u"DISPLAY_RESOLUTION_Y"].toInt();
            WARPING_MATRIX_INPUT_RESOLUTION_X = obj[u"WARPING_MATRIX_INPUT_RESOLUTION_X"].toInt();
            WARPING_MATRIX_INPUT_RESOLUTION_Y = obj[u"WARPING_MATRIX_INPUT_RESOLUTION_Y"].toInt();
            WARPING_MATRIX_COUNT = obj[u"WARPING_MATRIX_COUNT"].toInt();
            CONTENT_RESOLUTION_X = obj[u"CONTENT_RESOLUTION_X"].toInt();
            CONTENT_RESOLUTION_Y = obj[u"CONTENT_RESOLUTION_Y"].toInt();
            WARPING_MATRIX_EXTRAPOLATED_RESOLUTION_X = obj[u"WARPING_MATRIX_EXTRAPOLATED_RESOLUTION_X"].toInt();
            WARPING_MATRIX_EXTRAPOLATED_RESOLUTION_Y = obj[u"WARPING_MATRIX_EXTRAPOLATED_RESOLUTION_Y"].toInt();

            qCInfo(KWINARHUD_DEBUG) << "Loaded warping constants from" << f.fileName();
        }
        else
        {
            qCWarning(KWINARHUD_DEBUG) << "Error parsing contents of" << f.fileName() << ":" << parseError.errorString();
        }
    }
    else
    {
        qCWarning(KWINARHUD_DEBUG) << "Could not open warping constants file" << f.fileName() << "Using default values";
    }

    qCInfo(KWINARHUD_DEBUG) << "DISPLAY_RESOLUTION_X:" << DISPLAY_RESOLUTION_X;
    qCInfo(KWINARHUD_DEBUG) << "DISPLAY_RESOLUTION_Y:" << DISPLAY_RESOLUTION_Y;
    qCInfo(KWINARHUD_DEBUG) << "WARPING_MATRIX_INPUT_RESOLUTION_X:" << WARPING_MATRIX_INPUT_RESOLUTION_X;
    qCInfo(KWINARHUD_DEBUG) << "WARPING_MATRIX_INPUT_RESOLUTION_Y:" << WARPING_MATRIX_INPUT_RESOLUTION_Y;
    qCInfo(KWINARHUD_DEBUG) << "WARPING_MATRIX_COUNT:" << WARPING_MATRIX_COUNT;
    qCInfo(KWINARHUD_DEBUG) << "CONTENT_RESOLUTION_X:" << CONTENT_RESOLUTION_X;
    qCInfo(KWINARHUD_DEBUG) << "CONTENT_RESOLUTION_Y:" << CONTENT_RESOLUTION_Y;
    qCInfo(KWINARHUD_DEBUG) << "WARPING_MATRIX_EXTRAPOLATED_RESOLUTION_X:" << WARPING_MATRIX_EXTRAPOLATED_RESOLUTION_X;
    qCInfo(KWINARHUD_DEBUG) << "WARPING_MATRIX_EXTRAPOLATED_RESOLUTION_Y:" << WARPING_MATRIX_EXTRAPOLATED_RESOLUTION_Y;

    m_vertexCount = (WARPING_MATRIX_EXTRAPOLATED_RESOLUTION_X - 1) * (WARPING_MATRIX_EXTRAPOLATED_RESOLUTION_Y - 1) * 6;

    m_modelViewProjectioMatrixLocation  = m_shader->uniformLocation("modelViewProjectionMatrix");
    m_warpingMatrixTextureLocation      = m_shader->uniformLocation("warpingMatrixTexture");
    m_matrixCountLocation               = m_shader->uniformLocation("matrixCount");
    m_matrixResolutionLocation          = m_shader->uniformLocation("matrixResolution");
    m_matrixInterpolationFactorLocation = m_shader->uniformLocation("matrixInterpolationFactor");
    m_matrixInterpolationIndexLocation  = m_shader->uniformLocation("matrixInterpolationIndex");
    m_inputTextureLocation              = m_shader->uniformLocation("inputTexture");
    m_uvFunctLocation                   = m_shader->uniformLocation("uvFunc");

    m_warpedOutputManager = std::make_unique<MBitionWarpedOutputManager>(this);
}

ClassicArHudEffect::~ClassicArHudEffect() = default;

bool ClassicArHudEffect::isActive() const
{
    return m_warpedOutput != nullptr;
}

void ClassicArHudEffect::checkGlTexture(Output* screen)
{
    const QSize nativeSize = screen->geometry().size() * screen->scale();
    if (!m_GLtexture || m_GLtexture->size() != nativeSize)
    {
        qCInfo(KWINARHUD_DEBUG) << "Init texture and framebuffer";
        // If the texture doesn't exist or is of a different size, create a new one.
        m_GLtexture = GLTexture::allocate(GL_RGBA8, nativeSize);
        m_GLtexture->setFilter(GL_LINEAR);
        m_GLtexture->setWrapMode(GL_CLAMP_TO_EDGE);
        m_GLframebuffer.reset(new GLFramebuffer(m_GLtexture.get()));

        m_GLtexture->bind();
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        m_GLtexture->unbind();
    }
}

MBitionWarpedOutput* ClassicArHudEffect::warpedOutput(Output* screen)
  {
    if (!screen)
    {
        qCWarning(KWINARHUD_DEBUG) << "warpedOutput failed: provided screen is nullptr";
        return nullptr;
    }
    if (screen != m_warpedScreen && m_warpedOutput)
    {
        qCWarning(KWINARHUD_DEBUG) << "warpedOutput failed: provided screen is not warped screen. This effect only supports 1 screen.";
        return nullptr;
    }
    m_warpedScreen = screen;

    if (!m_warpedOutput)
    {
        qCInfo(KWINARHUD_DEBUG) << "Creating warping output for screen " << screen->name();
        m_warpedOutput.reset(new MBitionWarpedOutput());
        checkGlTexture(screen);
    }

    return m_warpedOutput.get();
}

/**
 * @brief Generates a vertex buffer object (VBO) to run shader on it.
 * @return A pointer to the generated GLVertexBuffer.
 */
static GLVertexBuffer* texturedRectVbo(const uint32_t vertexCount)
{
    // Create a streaming buffer for the VBO and reset it.
    GLVertexBuffer* vbo = GLVertexBuffer::streamingBuffer();
    if (!vbo)
    {
        qCWarning(KWINARHUD_DEBUG) << "texturedRectVbo failed: GLVertexBuffer::streamingBuffer() returned nullptr";
        return vbo;
    }
    vbo->reset();

    // Define vertex attribute layout.
    const GLVertexAttrib attribs[] = {
        {VA_Position, 2, GL_FLOAT, offsetof(GLVertex2D, position)},
        {VA_TexCoord, 2, GL_FLOAT, offsetof(GLVertex2D, texcoord)},
    };
    vbo->setAttribLayout(attribs, sizeof(GLVertex2D));

    const auto opt = vbo->map<GLVertex2D>(vertexCount);
    if (!opt) {
        return nullptr;
    }
    const auto map = *opt;

    // Define UV coordinates and step size for texturing.
    constexpr auto uv       = std::array<float, 4>{{1.0f, 1.0f, 0.0, 0.0}};
    QVector2D  stepTexcoord = {uv[0] / float(WARPING_MATRIX_EXTRAPOLATED_RESOLUTION_X - 1),
                               uv[1] / float(WARPING_MATRIX_EXTRAPOLATED_RESOLUTION_Y - 1)};

    // Generate vertices for the grid of rectangles.
    for (uint32_t x = 0; x < (WARPING_MATRIX_EXTRAPOLATED_RESOLUTION_X - 1); ++x)
    {
        for (uint32_t y = 0; y < (WARPING_MATRIX_EXTRAPOLATED_RESOLUTION_Y - 1); ++y)
        {
            const uint32_t start = x * (WARPING_MATRIX_EXTRAPOLATED_RESOLUTION_Y - 1) * 6 + y * 6;

            QVector2D orig        = {uv[2] + static_cast<float>(x) * stepTexcoord.x(),
                                     uv[3] + static_cast<float>(y) * stepTexcoord.y()};
            QVector2D texcoord_00 = orig;
            QVector2D texcoord_01 = orig + QVector2D(0, 1) * stepTexcoord;
            QVector2D texcoord_10 = orig + QVector2D(1, 0) * stepTexcoord;
            QVector2D texcoord_11 = orig + QVector2D(1, 1) * stepTexcoord;

            // Define vertices for two triangles forming a rectangle.
            // First triangle
            map[start + 0] = GLVertex2D{.position = QVector2D(0, 0), .texcoord = texcoord_01};
            map[start + 1] = GLVertex2D{.position = QVector2D(0, 0), .texcoord = texcoord_10};
            map[start + 2] = GLVertex2D{.position = QVector2D(0, 0), .texcoord = texcoord_00};
            // Second triangle
            map[start + 3] = GLVertex2D{.position = QVector2D(0, 0), .texcoord = texcoord_01};
            map[start + 4] = GLVertex2D{.position = QVector2D(0, 0), .texcoord = texcoord_11};
            map[start + 5] = GLVertex2D{.position = QVector2D(0, 0), .texcoord = texcoord_10};
        }
    }

    // Unmap the VBO memory and return the generated VBO.
    vbo->unmap();
    return vbo;
}

void ClassicArHudEffect::paintScreen(const RenderTarget &renderTarget, const RenderViewport &viewport, int mask, const QRegion &region, Output *screen)
{
    if (!effects) [[unlikely]]
    {
        qCWarning(KWINARHUD_DEBUG) << "paintScreen failed: effects is nullptr";
        return;
    }

    // Check if the screen is being warped, if not, skip the effect.
    if (screen != m_warpedScreen || !m_warpedOutput || !m_warpedOutput->isInitialized())
    {
        effects->paintScreen(renderTarget, viewport, mask, region, screen);
        return;
    }

    // Render the screen in an offscreen texture.
    checkGlTexture(screen);
    if (!m_GLframebuffer)
    {
        // if there is some problems with framebuffer, skip the effect
        qCWarning(KWINARHUD_DEBUG) << "paintScreen failed: framebuffer of screen is nullptr";
        effects->paintScreen(renderTarget, viewport, mask, region, screen);
        return;
    }

    GLFramebuffer::pushFramebuffer(m_GLframebuffer.get());
    effects->paintScreen(renderTarget, viewport, mask, region, screen);
    GLFramebuffer::popFramebuffer();

    // Projection matrix + rotate transform.
    const QMatrix4x4 modelViewProjectionMatrix(viewport.projectionMatrix());

    glActiveTexture(GL_TEXTURE0);
    m_GLtexture->bind();

    // Clear the background.
    glClearColor(0, 0, 0, 0);
    glClear(GL_COLOR_BUFFER_BIT);

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, m_warpedOutput->m_texture);

    int32_t index;
    float   factor;
    m_warpedOutput->m_matrixInterpolationModel.getInterpolationParameters(index, factor);

    ShaderManager* sm = ShaderManager::instance();
    sm->pushShader(m_shader.get());
    m_shader->setUniform(m_modelViewProjectioMatrixLocation, modelViewProjectionMatrix);
    m_shader->setUniform(m_inputTextureLocation, 0);
    m_shader->setUniform(m_warpingMatrixTextureLocation, 1);
    m_shader->setUniform(m_matrixCountLocation, int(Warping::WARPING_MATRIX_COUNT));
    m_shader->setUniform(m_matrixResolutionLocation,
                         {float(WARPING_MATRIX_EXTRAPOLATED_RESOLUTION_X), float(WARPING_MATRIX_EXTRAPOLATED_RESOLUTION_Y)});
    m_shader->setUniform(m_matrixInterpolationFactorLocation, factor);
    m_shader->setUniform(m_matrixInterpolationIndexLocation, index);
    std::array<float, 4> uvFunc = Warping::getUVFunc();
    m_shader->setUniform(m_uvFunctLocation, QVector4D(uvFunc[0], uvFunc[1], uvFunc[2], uvFunc[3]));

    GLVertexBuffer* vbo = texturedRectVbo(m_vertexCount);
    if (vbo)
    {
        vbo->bindArrays();
        vbo->draw(GL_TRIANGLES, 0, m_vertexCount);
        vbo->unbindArrays();
    }
    else
    {
        qCWarning(KWINARHUD_DEBUG) << "paintScreen failed: texturedRectVbo() returned nullptr";
    }

    sm->popShader();

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, 0);

    m_GLtexture->unbind();

    // TODO: Could be transformed with the rendering
    effects->addRepaint(screen->geometry());
}

}  // namespace KWin
