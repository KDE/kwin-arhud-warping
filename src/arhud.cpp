/*
 * SPDX-FileCopyrightText: Copyright (c) 2017-2022, Daimler AG
 * SPDX-FileCopyrightText: Copyright (c) 2022, Mercedes-Benz AG
 * SPDX-FileCopyrightText: Copyright (c) 2022 MBition GmbH.
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "arhud.h"
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
#include "qwayland-server-mbition-warped-output-unstable-v1.h"

#include <memory>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include "kwinarhud_debug.h"

static const int s_version = 1;

// TODO Make sure that wayland callbacks and paintScreen get called from the same threads
class MBitionWarpedOutput : public QtWaylandServer::zmbition_warped_output_v1
{
public:
  MBitionWarpedOutput()
      : QtWaylandServer::zmbition_warped_output_v1(),
        m_calibratedHeadPositions(WARPING_MATRIX_COUNT),
        m_matrixInterpolationModel(WARPING_MATRIX_COUNT),
        m_matrixTextureModel(WARPING_MATRIX_COUNT,
                             WARPING_MATRIX_EXTRAPOLATED_RESOLUTION_X,
                             WARPING_MATRIX_EXTRAPOLATED_RESOLUTION_Y)
  {
    glGenTextures(1, &m_texture);
    if (m_texture == GL_NONE)
    {
      qCWarning(KWINARHUD_DEBUG) << "failed to create texture";
    }

    m_calibratedMatrices.reserve(WARPING_MATRIX_COUNT);
    for (uint32_t i = 0; i < WARPING_MATRIX_COUNT; i++)
    {
      m_calibratedMatrices.emplace_back(WARPING_MATRIX_EXTRAPOLATED_RESOLUTION_X,
                                        WARPING_MATRIX_EXTRAPOLATED_RESOLUTION_Y);
    }
  }

  /**
   * @brief Setting head position taken from ArHudDiagnosis
   * This function is a callback function called by wayland and stores values in matrix interpolation model
   * @param position [in] - Real head position data.
   */
  void zmbition_warped_output_v1_set_head_position(Resource* /*resource*/, wl_array* position) override
  {
    qCDebug(KWINARHUD_DEBUG) << "setting new head position";

    if (!position)
    {
      qCWarning(KWINARHUD_DEBUG) << "setting new head position failed. invalid position";
      return;
    }

    WarpingMatrixInterpolationModel::Position headPos;
    readHeadPosition(headPos, position);
    m_matrixInterpolationModel.setEyePosition(headPos);
  }

  /**
   * @brief Setting warping matrices and reference eye position taken from ArHudDiagnosis
   * This function is called by incoming wayland package, stores data in local model.
   * @param resource [in] - An object that clients and the compositor use to communicate with each other.
   * @param index [in] - Specify which matrix should be bound to texture. (Upper, middle, lower)
   * Index is expected to be in range [0-2]
   * @param head_position [in] - Setting ReferenceEyePosition
   * @param matrix [in] - Warping matrix
   */
  void zmbition_warped_output_v1_set_warping_matrix(Resource* resource,
                                                    uint32_t  index,
                                                    wl_array* head_position,
                                                    wl_array* matrix) override
  {
    qCInfo(KWINARHUD_DEBUG) << "setting new matrices";
    if (!resource)
    {
      qCWarning(KWINARHUD_DEBUG) << "setting new matrices failed. invalid resource";
      return;
    }
    if (!head_position)
    {
      qCWarning(KWINARHUD_DEBUG) << "setting new matrices failed. invalid head_position";
      return;
    }
    if (!matrix)
    {
      qCWarning(KWINARHUD_DEBUG) << "setting new matrices failed. invalid matrix";
      return;
    }

    if (index >= Warping::WARPING_MATRIX_COUNT)
    {
      wl_resource_post_error(resource->handle,
                             error_index_out_of_bounds,
                             "Warping matrix index out of bounds (Should be 0-matrixCount)");
      return;
    }

    readHeadPosition(m_calibratedHeadPositions[index], head_position);
    readWarpingMatrix(m_calibratedMatrices[index], matrix);

    setMatrix(index);
  }

  /**
   * @brief Destroying the wayland resource
   * @param resource [in] - The existing resource between client and compositor that should be destroy.
   */
  void zmbition_warped_output_v1_destroy(Resource* resource) override
  {
    if (!resource)
    {
      qCWarning(KWINARHUD_DEBUG) << "destroy resource failed. invalid resource";
      return;
    }
    wl_resource_destroy(resource->handle);
  }

private:
  /**
   * @brief Bind a specific Warping matrix based on it's index to a GL_Texture_2D
   * @param index [in] - Specify which matrix should be bound to texture. (Upper, middle, lower)
   */
  void setMatrix(uint32_t index)
  {
    // TODO: m_calibratedMatrices can probably go
    if (index >= static_cast<int>(sizeof(m_calibratedMatrices)))
    {
      qCWarning(KWINARHUD_DEBUG) << "setMatrix failed. index out of bounds: " << index
                                 << ", count of calibrated matrices: " << sizeof(m_calibratedMatrices);
      return;
    }
    if (index >= static_cast<int>(sizeof(m_calibratedHeadPositions)))
    {
      qCWarning(KWINARHUD_DEBUG) << "setMatrix failed. index out of bounds: " << index
                                 << ", count of calibrated head positions: " << sizeof(m_calibratedHeadPositions);
      return;
    }
    m_matrixTextureModel.setMatrix(index, m_calibratedMatrices[index]);
    m_matrixInterpolationModel.setReferenceEyePosition(index, m_calibratedHeadPositions[index]);

    m_initialized |= 1 << index;
    if (isInitialized())
    {
      glBindTexture(GL_TEXTURE_2D, m_texture);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
      auto textureData = m_matrixTextureModel.getTextureData();

      glTexImage2D(GL_TEXTURE_2D,
                   0,
                   GL_RGBA8,
                   WARPING_MATRIX_EXTRAPOLATED_RESOLUTION_X * 2,
                   WARPING_MATRIX_EXTRAPOLATED_RESOLUTION_Y * WARPING_MATRIX_COUNT,
                   0,
                   GL_RGBA,
                   GL_UNSIGNED_BYTE,
                   textureData.data());
      glBindTexture(GL_TEXTURE_2D, 0);
      qCInfo(KWINARHUD_DEBUG) << "SetMatrix: all matrices set, MBitionWarpedOutput is initialized";
    }
  }

  /**
   * @brief Read a wayland array of floats and store them in a destination array as head positions
   * @param input [in] - Read a wayland array of floats
   * @param destination [out] - Store input data in a destination array of floats
   */
  static void readHeadPosition(WarpingMatrixInterpolationModel::Position& destination, wl_array* input)
  {
    if (!input)
    {
      qCWarning(KWINARHUD_DEBUG) << "readHeadPosition failed: input array missing";
      return;
    }
    constexpr size_t expectedSize = sizeof(float) * 3;
    if (input->size != expectedSize)
    {
      qCWarning(KWINARHUD_DEBUG) << "Invalid size for head position. Actual size:" << input->size
                                 << ", Expected size:" << expectedSize;
      return;
    }

    float* dataArray = static_cast<float*>(input->data);
    destination      = {{dataArray[0], dataArray[1], dataArray[2]}};
  }

  /**
   * @brief Read a wayland array of floats and construct an ExtendedWarpingMatrix
   * @param input [in] - Read a wayland array of floats
   * @param destination [out] - Store converted input data in an ExtendedWarpingMatrix
   */
  static void readWarpingMatrix(Matrix& destination, wl_array* input)
  {
    size_t expectedSize = sizeof(float) * WARPING_MATRIX_INPUT_RESOLUTION_X * WARPING_MATRIX_INPUT_RESOLUTION_Y * 2;
    if (input->size != expectedSize)
    {
      qCWarning(KWINARHUD_DEBUG) << "Invalid size for warping matrices. Actual size:" << input->size
                                 << ", Expected size:" << expectedSize;
      return;
    }

    auto calibratedMatrices = static_cast<float*>(input->data);

    Matrix intermediate(WARPING_MATRIX_INPUT_RESOLUTION_X, WARPING_MATRIX_INPUT_RESOLUTION_Y, calibratedMatrices);
    intermediate.getExtendedWarpingMatrix({float64_t(DISPLAY_RESOLUTION_X), float64_t(DISPLAY_RESOLUTION_Y)}, destination);
  }

public:
  bool isInitialized() const
  {
    return m_initialized == ((1u << WARPING_MATRIX_COUNT) - 1);
  }

  GLuint                   m_texture     = 0;
  uint32_t                 m_initialized = 0;
  WarpingMatrixInterpolationModel::Position m_headPosition;
  std::vector<WarpingMatrixInterpolationModel::Position> m_calibratedHeadPositions;
  std::vector<Matrix>      m_calibratedMatrices;
  WarpingMatrixInterpolationModel m_matrixInterpolationModel;

  MatrixTextureModel m_matrixTextureModel;
};

class MBitionWarpedOutputManager : public QtWaylandServer::zmbition_warped_output_manager_v1
{
public:
  MBitionWarpedOutputManager(KWin::ArHUDEffect* effect)
      : QtWaylandServer::zmbition_warped_output_manager_v1(*KWin::effects->waylandDisplay(), s_version)
      , m_effect(effect)
  {
  }

  /**
   * @brief Get the warped output for the existing wayland resource and output ID.
   *
   * @param resource [in] - A pointer to the resource associated with the client.
   * @param id [in] - The ID of the resource to be added to the warped output.
   * @param output [in] - A pointer to the Wayland resource representing the output.
   */
  void zmbition_warped_output_manager_v1_get_warped_output(Resource* resource,
                                                           uint32_t  id,
                                                           struct ::wl_resource* /*output*/) override
  {
    qCInfo(KWINARHUD_DEBUG) << "Incoming request get_warped_output with id " << id;
    if (!resource)
    {
      qCWarning(KWINARHUD_DEBUG) << "resource is nullptr";
      return;
    }
    if (!m_effect)
    {
      qCWarning(KWINARHUD_DEBUG) << "Effect m_effect is nullptr";
      return;
    }

    // find hud screen by its resolution
    const auto& screens = KWin::effects->screens();

    int screenIndex = 0;
    KWin::Output* screenPtr = nullptr;

    for (auto screen : screens)
    {
      if (screen)
      {
        qCInfo(KWINARHUD_DEBUG) << "Screen " << screenIndex << ": " << screen->manufacturer() << ", " << screen->model()
                                << ", " << screen->name() << ", " << screen->geometry();

        if (uint32_t(screen->geometry().width()) == DISPLAY_RESOLUTION_X && uint32_t(screen->geometry().height()) == DISPLAY_RESOLUTION_Y)
        {
          screenPtr = screen;
          break;
        }
      }
      else
      {
        qCWarning(KWINARHUD_DEBUG) << "Printing screen infos failed: screen " << screenIndex << " is nullptr";
      }
      screenIndex++;
    }

    if (screenPtr != nullptr)
    {
      auto warpedOutput = m_effect->warpedOutput(screenPtr);
      if (warpedOutput)
      {
        warpedOutput->add(resource->client(), id, s_version);
        qCInfo(KWINARHUD_DEBUG) << "Screen " << screenPtr->name() << " " << screenPtr->geometry()
                                << " is added to the warping plugin";
      }
      else
      {
        qCWarning(KWINARHUD_DEBUG) << "warpedOutput is nullptr";
      }
    }
    else
    {
      qCWarning(KWINARHUD_DEBUG) << "Could not get warped output";
    }
  }

private:
  KWin::ArHUDEffect* const m_effect;
};

namespace KWin
{

  ArHUDEffect::ArHUDEffect()
      : Effect()
      , m_shader(
            ShaderManager::instance()->generateShaderFromFile(ShaderTrait::MapTexture,
                                                              QStringLiteral(":/effects/arhud/shaders/warping.vert"),
                                                              QStringLiteral(":/effects/arhud/shaders/warping.frag")))
  {
    if (!m_shader->isValid())
    {
      qCWarning(KWINARHUD_DEBUG) << "Shader is not valid!";
      return;
    }

    qCInfo(KWINARHUD_DEBUG) << "Loading ArHUDEffect";

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

  ArHUDEffect::~ArHUDEffect() = default;

  bool ArHUDEffect::supported()
  {
    return effects->compositingType() == OpenGLCompositing && effects->waylandDisplay();
  }

  bool ArHUDEffect::isActive() const
  {
    return m_warpedOutput != nullptr;
  }

  void ArHUDEffect::checkGlTexture(Output* screen)
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

  MBitionWarpedOutput* ArHUDEffect::warpedOutput(Output* screen)
  {
    if (!screen)
    {
      qCWarning(KWINARHUD_DEBUG) << "warpedOutput failed: provided screen is nullptr";
      return nullptr;
    }
    if (screen != m_warpedScreen && m_warpedOutput)
    {
      qCWarning(KWINARHUD_DEBUG)
          << "warpedOutput failed: provided screen is not warped screen. This effect only supports 1 screen.";
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

  /**
   * @brief paint something on top of the windows. (one or multiple drawing and effects).
   * @param mask [in] - A set of flags that control or specify various options or conditions for painting the screen.
   * There are some enums for this purpose in kwineffects.h This function is called by wayland in every render loop.
   * @param region [in] - Specifying areas of a graphical user interface that need to be painted or updated.
   * @param data [in] - A set of data for painting the window/effects on the screen(tranlation, rotation, scale, ...).
   */
  void ArHUDEffect::paintScreen(const RenderTarget &renderTarget, const RenderViewport &viewport, int mask, const QRegion &region, Output *screen)
  {
    if (!effects)
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
