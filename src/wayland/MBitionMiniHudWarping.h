// SPDX-FileCopyrightText: Copyright (c) 2025 MBition GmbH.
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "qwayland-server-mbition-mini-hud-warping-unstable-v1.h"

#include "kwinarhud_debug.h"

#include <QString>

namespace KWin
{
    class DefaultHudEffect;
}

class MBitionMiniHudWarping : public QtWaylandServer::mbition_mini_hud_warping_v1
{
public:
    MBitionMiniHudWarping(KWin::DefaultHudEffect* hud_effect);

    void mbition_mini_hud_warping_v1_destroy(Resource* resource) override;

    void mbition_mini_hud_warping_v1_setMatrices(Resource* resource, int fd) override;
    void mbition_mini_hud_warping_v1_setMirrorLevel(Resource* resource, wl_fixed_t mirrorLevel) override;
    void mbition_mini_hud_warping_v1_setWhitePoint(Resource*resource, unsigned int red, unsigned int green, unsigned int blue, unsigned int divisor) override;

private:
    KWin::DefaultHudEffect* m_effect;
};
