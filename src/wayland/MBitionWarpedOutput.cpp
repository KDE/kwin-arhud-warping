// SPDX-FileCopyrightText: Copyright (c) 2025 MBition GmbH.
// SPDX-License-Identifier: GPL-2.0-or-later

#include "MBitionWarpedOutput.h"

#include "MatrixTextureModel.hxx"
#include "WarpingMatrixInterpolationModel.hxx"
#include "WarpingUtils.hxx"
#include "classicArHud.h"

MBitionWarpedOutput::MBitionWarpedOutput()
    : QtWaylandServer::zmbition_warped_output_v1(),
    m_texture(0),
    m_initialized(0),
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

void MBitionWarpedOutput::zmbition_warped_output_v1_set_head_position(Resource* /*resource*/, wl_array* position)
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

void MBitionWarpedOutput::zmbition_warped_output_v1_set_warping_matrix(Resource* resource,
                                                                       uint32_t  index,
                                                                       wl_array* head_position,
                                                                       wl_array* matrix)
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

void MBitionWarpedOutput::zmbition_warped_output_v1_destroy(Resource* resource)
{
    if (!resource)
    {
        qCWarning(KWINARHUD_DEBUG) << "destroy resource failed. invalid resource";
        return;
    }
    wl_resource_destroy(resource->handle);
}

void MBitionWarpedOutput::setMatrix(uint32_t index)
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

void MBitionWarpedOutput::readHeadPosition(WarpingMatrixInterpolationModel::Position& destination, wl_array* input)
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

void MBitionWarpedOutput::readWarpingMatrix(Matrix& destination, wl_array* input)
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

bool MBitionWarpedOutput::isInitialized() const
{
    return m_initialized == ((1u << WARPING_MATRIX_COUNT) - 1);
}
