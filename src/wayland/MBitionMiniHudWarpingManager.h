// SPDX-FileCopyrightText: Copyright (c) 2025 MBition GmbH.
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "qwayland-server-mbition-mini-hud-warping-unstable-v1.h"

#include <effect/effect.h>

#include "kwinarhud_debug.h"

namespace KWin
{
    class DefaultHudEffect;
}

class MBitionMiniHudWarpingManager : public QtWaylandServer::mbition_mini_hud_warping_manager_v1
{
public:
    MBitionMiniHudWarpingManager(KWin::DefaultHudEffect* effect);

    void mbition_mini_hud_warping_manager_v1_get_mini_hud(Resource* resource, uint32_t id,
                                                          unsigned int displayWidth, unsigned int displayHeight,
                                                          unsigned int appAreaWidth, unsigned int appAreaHeight) override;

public:
    KWin::DefaultHudEffect* const m_effect;
    KWin::Output* m_screen;
};
