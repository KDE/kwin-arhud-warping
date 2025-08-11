// SPDX-FileCopyrightText: Copyright (c) 2025 MBition GmbH.
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "qwayland-server-mbition-warped-output-unstable-v1.h"

#include <effect/effect.h>

#include "kwinarhud_debug.h"

namespace KWin
{
    class ClassicArHudEffect;
}

class MBitionWarpedOutputManager : public QtWaylandServer::zmbition_warped_output_manager_v1
{
public:
    MBitionWarpedOutputManager(KWin::ClassicArHudEffect* effect);

    /**
     * @brief Get the warped output for the existing wayland resource and output ID.
     *
     * @param[in] resource - A pointer to the resource associated with the client.
     * @param[in] id - The ID of the resource to be added to the warped output.
     * @param[in] output - A pointer to the Wayland resource representing the output.
     */
    void zmbition_warped_output_manager_v1_get_warped_output(Resource* resource,
                                                             uint32_t  id,
                                                             struct ::wl_resource* /*output*/) override;

private:
    KWin::ClassicArHudEffect* const m_effect;
};
