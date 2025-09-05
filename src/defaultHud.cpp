// SPDX-FileCopyrightText: Copyright (c) 2025 MBition GmbH.
// SPDX-License-Identifier: GPL-2.0-or-later

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <fcntl.h>
#include <unistd.h>
#include <cerrno>
#include <cstring>

#include "defaultHud.h"
#include "warpingEffect.h"
#include "kwinarhud_debug.h"

#include <opengl/glshader.h>
#include <opengl/glvertexbuffer.h>
#include <opengl/glshadermanager.h>
#include <opengl/gltexture.h>
#include <opengl/glframebuffer.h>
#include <effect/effecthandler.h>
#include <core/output.h>
#include <core/rendertarget.h>
#include <core/renderviewport.h>

#include "MBitionMiniHudWarping.h"
#include "MBitionMiniHudWarpingManager.h"

#include <QVector4D>
#include <QMatrix4x4>
#include <algorithm>
#include <QFile>
#include <QString>

struct ShaderRegion
{
    std::unique_ptr<KWin::GLVertexBuffer> m_vbo;
    QMatrix4x4 params1_x;
    QMatrix4x4 params1_y;
    QMatrix4x4 params2_x;
    QMatrix4x4 params2_y;
    QMatrix4x4 params3_x;
    QMatrix4x4 params3_y;
    QVector4D uv_span;

    ShaderRegion(uint32_t x, uint32_t y, const QVector4D& uvspan, const params_t& params1,  const params_t& params2, const params_t& params3)
        : m_vbo{ std::make_unique<KWin::GLVertexBuffer>(KWin::GLVertexBuffer::UsageHint::Static) }
        , params1_x{ getMatrix(x, y, 0, params1) }
        , params1_y{ getMatrix(x, y, 1, params1) }
        , params2_x{ getMatrix(x, y, 0, params2) }
        , params2_y{ getMatrix(x, y, 1, params2) }
        , params3_x{ getMatrix(x, y, 0, params3) }
        , params3_y{ getMatrix(x, y, 1, params3) }
        , uv_span{ uvspan }
    {
        setupVBO();

        const std::array<const QMatrix4x4*, 6> params = { &params1_x, &params1_y, &params2_x, &params2_y, &params3_x, &params3_y };
        for (int i = 0; i < 6; i++) {
            QString buffer;
            buffer.reserve(256);
            QTextStream logStream(&buffer);
            logStream << "[";
            const float* p = params[i]->data();
            for (int j = 0; j < 16; j++) {
                logStream << p[i] << ", ";
            }
            logStream << "]";
            qCInfo(KWINARHUD_DEBUG) << "region:" << x << " " << y << "| params:" << (i / 2 + 1) << (i % 2 ? "y" : "x") << *logStream.string();
        }
    }

    ShaderRegion(ShaderRegion&& other)
        : m_vbo{ std::move(other.m_vbo) }
        , params1_x{ other.params1_x }
        , params1_y{ other.params1_y }
        , params2_x{ other.params2_x }
        , params2_y{ other.params2_y }
        , params3_x{ other.params3_x }
        , params3_y{ other.params3_y }
        , uv_span{ other.uv_span }
    {}

    QMatrix4x4 getMatrix(uint32_t x, uint32_t y, uint32_t t, const std::array<float, 378>& a)
    {
        const uint32_t s = 42;
        const uint32_t p = 2 * x + s * y + t;
        return QMatrix4x4{
            a[p],         a[p + 2],         a[p + 4],         a[p + 6],
            a[p + s],     a[p + s + 2],     a[p + s + 4],     a[p + s + 6],
            a[p + 2 * s], a[p + 2 * s + 2], a[p + 2 * s + 4], a[p + 2 * s + 6],
            a[p + 3 * s], a[p + 3 * s + 2], a[p + 3 * s + 4], a[p + 3 * s + 6]
        };
    }

    void setupVBO()
    {
        // Shader attribute layout
        const KWin::GLVertexAttrib attribs[] = {
            { KWin::VA_Position, 2, GL_FLOAT, offsetof(KWin::GLVertex2D, position) }
        };
        m_vbo->setAttribLayout(attribs, sizeof(KWin::GLVertex2D));

        const auto map_ptr = m_vbo->map<KWin::GLVertex2D>(vertexDimensions);
        if (!map_ptr) {
            qCWarning(KWINARHUD_DEBUG) << "setupVBO failed - GLVertexBuffer::map() returned nullptr!";
            return;
        }
        const auto map = *map_ptr;

        // Define triangles
        uint32_t t = 0;
        QVector2D step = { 1.f / region_width, 1.f / region_height };
        for (uint32_t i = 0; i < region_width; i++) {
            for (uint32_t j = 0; j < region_height; j++) {
                QVector2D top_left =     {  i      * step.x(),  j      * step.y() };
                QVector2D top_right =    { (i + 1) * step.x(),  j      * step.y() };
                QVector2D bottom_left =  {  i      * step.x(), (j + 1) * step.y() };
                QVector2D bottom_right = { (i + 1) * step.x(), (j + 1) * step.y() };

                // First triangle
                map[t++].position = top_left;
                map[t++].position = bottom_left;
                map[t++].position = top_right;

                // Second triangle
                map[t++].position = top_right;
                map[t++].position = bottom_left;
                map[t++].position = bottom_right;
            }
        }

        m_vbo->unmap();
    }

    static constexpr uint32_t region_width = 3;
    static constexpr uint32_t region_height = 3;
    static constexpr uint32_t vertexDimensions = region_width * region_height * 6;
    static constexpr float max_x = 20.0f;
    static constexpr float max_y = 8.0f;
    static constexpr int32_t total_regions = 21;
};

namespace KWin
{

DefaultHudEffect::DefaultHudEffect()
    : m_vertexDimensions{ 0 }
    , m_whitePoint{ 1.0f, 1.0f, 1.0f }
    , m_mirrorLevel{ 5.0f }
    , m_screen{ nullptr }
    , m_shader(ShaderManager::instance()->generateShaderFromFile(ShaderTrait::MapTexture,
                                                                 QStringLiteral(":/effects/arhud/shaders/warping_default.vert"),
                                                                 QStringLiteral(":/effects/arhud/shaders/warping_default.frag")))
{
    if (!m_shader->isValid())
    {
        qCWarning(KWINARHUD_DEBUG) << "Shader is not valid!";
        return;
    }

    qCInfo(KWINARHUD_DEBUG) << "Loading DefaultHudEffect";

    m_params1_x_location = m_shader->uniformLocation("params1_x");
    m_params1_y_location = m_shader->uniformLocation("params1_y");
    m_params2_x_location = m_shader->uniformLocation("params2_x");
    m_params2_y_location = m_shader->uniformLocation("params2_y");
    m_params3_x_location = m_shader->uniformLocation("params3_x");
    m_params3_y_location = m_shader->uniformLocation("params3_y");
    m_uv_span_location = m_shader->uniformLocation("uv_span");
    m_mirrorLevel_location = m_shader->uniformLocation("mirrorLevel");
    m_whitePointCorrection_location = m_shader->uniformLocation("whitePointCorrection");
    m_source_location = m_shader->uniformLocation("source");
    m_window_size_location = m_shader->uniformLocation("window_size");

    m_miniHudManager = std::make_unique<MBitionMiniHudWarpingManager>(this);
}

DefaultHudEffect::~DefaultHudEffect() = default;

bool DefaultHudEffect::isActive() const
{
    return m_miniHud != nullptr;
}

void DefaultHudEffect::paintScreen(const RenderTarget& renderTarget, const RenderViewport& renderViewport, int mask, const QRegion& region, Output* screen)
{
    if (!effects) [[unlikely]]
    {
        qCWarning(KWINARHUD_DEBUG) << "paintScreen failed - effects is nullptr!";
        return;
    }

    if (screen != m_miniHudManager->m_screen)
    {
        effects->paintScreen(renderTarget, renderViewport, mask, region, screen);
        return;
    }

    if (m_shaderRegions.empty())
    {
        qCWarning(KWINARHUD_DEBUG) << "paintScreen failed - m_shaderRegions is empty!";
        return;
    }

    checkGlTexture();
    if (!m_framebuffer)
    {
        qCWarning(KWINARHUD_DEBUG) << "paintScreen failed - framebuffer is nullptr!";
        effects->paintScreen(renderTarget, renderViewport, mask, region, screen);
        return;
    }

    GLFramebuffer::pushFramebuffer(m_framebuffer.get());
    effects->paintScreen(renderTarget, renderViewport, mask, region, screen);
    GLFramebuffer::popFramebuffer();

    glActiveTexture(GL_TEXTURE0);
    m_texture->bind();

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glClearColor(0, 0, 0, 0);
    glClear(GL_COLOR_BUFFER_BIT);

    ShaderManager* sm = ShaderManager::instance();
    sm->pushShader(m_shader.get());

    for (uint32_t i = 0; i < ::ShaderRegion::total_regions; i++)
    {
        m_shader->setUniform(m_params1_x_location, m_shaderRegions[i].params1_x);
        m_shader->setUniform(m_params1_y_location, m_shaderRegions[i].params1_y);
        m_shader->setUniform(m_params2_x_location, m_shaderRegions[i].params2_x);
        m_shader->setUniform(m_params2_y_location, m_shaderRegions[i].params2_y);
        m_shader->setUniform(m_params3_x_location, m_shaderRegions[i].params3_x);
        m_shader->setUniform(m_params3_y_location, m_shaderRegions[i].params3_y);
        m_shader->setUniform(m_uv_span_location, m_shaderRegions[i].uv_span);
        m_shader->setUniform(m_mirrorLevel_location, m_mirrorLevel);
        m_shader->setUniform(m_whitePointCorrection_location, m_whitePoint);
        m_shader->setUniform(m_source_location, 0);
        m_shader->setUniform(m_window_size_location, QVector2D{ static_cast<float>(m_hudSize.displayWidth),
                                                                static_cast<float>(m_hudSize.displayHeight) });

        m_shaderRegions[i].m_vbo->bindArrays();
        m_shaderRegions[i].m_vbo->draw(GL_TRIANGLES, 0, ShaderRegion::vertexDimensions);
        m_shaderRegions[i].m_vbo->unbindArrays();
    }

    sm->popShader();

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, 0);
    m_texture->unbind();

    effects->addRepaint(screen->geometry());
}

void DefaultHudEffect::checkGlTexture()
{
    const QSize content_size{ static_cast<int>(m_hudSize.displayWidth), static_cast<int>(m_hudSize.displayHeight) };
    if (!m_texture || m_texture->size() != content_size)
    {
        qCInfo(KWINARHUD_DEBUG) << "Setting texture and framebuffer with size:" << content_size.width() << content_size.height();
        m_texture = GLTexture::allocate(GL_RGBA8, content_size);
        m_texture->setFilter(GL_LINEAR);
        m_texture->setWrapMode(GL_CLAMP_TO_EDGE);
        m_framebuffer = std::make_unique<GLFramebuffer>(m_texture.get());

        m_texture->bind();
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        m_texture->unbind();
    }
}

MBitionMiniHudWarping* DefaultHudEffect::miniHud(Output* screen)
{
    qCInfo(KWINARHUD_DEBUG) << "miniHud()";
    assert(screen != nullptr);

    if (screen != m_screen && m_miniHud)
    {
        qCWarning(KWINARHUD_DEBUG) << "miniHud() failed: wrong screen!";
        return nullptr;
    }
    m_screen = screen;

    if (!m_miniHud)
    {
        qCInfo(KWINARHUD_DEBUG) << "Creating mini hud for screen " << screen->name();
        m_miniHud.reset(new MBitionMiniHudWarping(this));
        checkGlTexture();
    }

    return m_miniHud.get();
}

void DefaultHudEffect::setHudSize(unsigned int displayWidth, unsigned int displayHeight, unsigned int appAreaWidth, unsigned int appAreaHeight)
{
    m_hudSize.displayWidth = displayWidth;
    m_hudSize.displayHeight = displayHeight;
    m_hudSize.appAreaWidth = appAreaWidth;
    m_hudSize.appAreaHeight = appAreaHeight;
}

void DefaultHudEffect::setMatrices(int fd)
{
    std::array<params_t, 3> params;
    ssize_t data_size = 378 * sizeof(float);
    lseek(fd, 0, SEEK_SET);
    for (params_t& param : params) {
        ssize_t bytes_read = read(fd, param.data(), data_size);

        if (bytes_read == 0) {
            char buf[256];
            char* msg = strerror_r(errno, buf, sizeof(buf));
            qCWarning(KWINARHUD_DEBUG) << "Error reading data from the file descriptor ..." << msg;
            close(fd);
            return;
        }
    }

    close(fd);
    qCInfo(KWINARHUD_DEBUG) << "setMatrices";
    setupShaderRegions(params[0], params[1], params[2]);
}

void DefaultHudEffect::setMirrorLevel(float mirrorLevel)
{
    qCInfo(KWINARHUD_DEBUG) << "setMirrorLevel, mirrorLevel=" << mirrorLevel;
    m_mirrorLevel = mirrorLevel;
}

void DefaultHudEffect::setWhitePoint(float red, float green, float blue)
{
    qCInfo(KWINARHUD_DEBUG) << "setWhitePoint, red=" << red << ", green=" << green << ", blue=" << blue;
    m_whitePoint.setX(red);
    m_whitePoint.setY(green);
    m_whitePoint.setZ(blue);
}

void DefaultHudEffect::setupShaderRegions(const params_t& params1, const params_t& params2, const params_t& params3)
{
    m_shaderRegions.clear();

    QVector2D margins = { (m_hudSize.displayWidth - m_hudSize.appAreaWidth) / 2.f,
                          (m_hudSize.displayHeight - m_hudSize.appAreaHeight) / 2.f };
    QVector4D content_area = { margins.x() / m_hudSize.displayWidth,
                               margins.y() / m_hudSize.displayHeight,
                               (m_hudSize.displayWidth - margins.x()) / m_hudSize.displayWidth,
                               (m_hudSize.displayHeight - margins.y()) / m_hudSize.displayHeight };

    qCInfo(KWINARHUD_DEBUG) << "content_area: [" << content_area.x() << "-" << content_area.z() << "] [" <<content_area.y() << "-" << content_area.w() << "]";

    for (int32_t index = 0; index < ::ShaderRegion::total_regions; index++)
    {
        uint32_t x = uint32_t{ static_cast<uint32_t>(std::max(int32_t{ 0 }, (index % 7) * 3 - 1)) };
        uint32_t y = uint32_t{ static_cast<uint32_t>(std::max(int32_t{ 0 }, (index / 7) * 3 - 1)) };

        QVector4D uv_span = { content_area.x() + (x / 20.f) * (content_area.z() - content_area.x()),
                              content_area.y() + (y / 8.f) * (content_area.w() - content_area.y()),
                              content_area.x() + ((x + 3) / 20.f) * (content_area.z() - content_area.x()),
                              content_area.y() + ((y + 3) / 8.f) * (content_area.w() - content_area.y()) };

        m_shaderRegions.emplace_back(x, y, uv_span, params1, params2, params3);
    }
}

} // namespace KWin
