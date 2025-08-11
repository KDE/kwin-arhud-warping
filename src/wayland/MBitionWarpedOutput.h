// SPDX-FileCopyrightText: Copyright (c) 2025 MBition GmbH.
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "qwayland-server-mbition-warped-output-unstable-v1.h"
#include "kwinarhud_debug.h"

#include "WarpingMatrixInterpolationModel.hxx"
#include "MatrixTextureModel.hxx"
#include "WarpingUtils.hxx"

#include <opengl/gltexture.h>
#include <vector>

class MBitionWarpedOutput : public QtWaylandServer::zmbition_warped_output_v1
{
public:
    MBitionWarpedOutput();

    /**
     * @brief Setting head position taken from ArHudDiagnosis
     * This function is a callback function called by wayland and stores values in matrix interpolation model
     * @param[in] position - Real head position data.
     */
    void zmbition_warped_output_v1_set_head_position(Resource* /*resource*/, wl_array* position) override;

    /**
     * @brief Setting warping matrices and reference eye position taken from ArHudDiagnosis
     * This function is called by incoming wayland package, stores data in local model.
     * @param[in] resource - An object that clients and the compositor use to communicate with each other.
     * @param[in] index - Specify which matrix should be bound to texture. (Upper, middle, lower)
     * Index is expected to be in range [0-2]
     * @param[in] head_position - Setting ReferenceEyePosition
     * @param[in] matrix - Warping matrix
     */
    void zmbition_warped_output_v1_set_warping_matrix(Resource* resource,
                                                      uint32_t  index,
                                                      wl_array* head_position,
                                                      wl_array* matrix) override;

    /**
     * @brief Destroying the wayland resource
     * @param[in] resource - The existing resource between client and compositor that should be destroy.
     */
    void zmbition_warped_output_v1_destroy(Resource* resource) override;

private:
    /**
     * @brief Bind a specific Warping matrix based on it's index to a GL_Texture_2D
     * @param[in] index - Specify which matrix should be bound to texture. (Upper, middle, lower)
     */
    void setMatrix(uint32_t index);

    /**
     * @brief Read a wayland array of floats and store them in a destination array as head positions
     * @param[in] input - Read a wayland array of floats
     * @param[out] destination - Store input data in a destination array of floats
     */
    static void readHeadPosition(WarpingMatrixInterpolationModel::Position& destination, wl_array* input);

    /**
     * @brief Read a wayland array of floats and construct an ExtendedWarpingMatrix
     * @param[in] input - Read a wayland array of floats
     * @param[out] destination - Store converted input data in an ExtendedWarpingMatrix
     */
    static void readWarpingMatrix(Matrix& destination, wl_array* input);

public:
    bool isInitialized() const;

    GLuint m_texture;
    uint32_t m_initialized;
    WarpingMatrixInterpolationModel::Position m_headPosition;
    std::vector<WarpingMatrixInterpolationModel::Position> m_calibratedHeadPositions;
    std::vector<Matrix> m_calibratedMatrices;
    WarpingMatrixInterpolationModel m_matrixInterpolationModel;

    MatrixTextureModel m_matrixTextureModel;
};
