// SPDX-FileCopyrightText: Copyright (c) 2025 MBition GmbH.
// SPDX-License-Identifier: GPL-2.0-or-later

#include "MBitionMiniHudWarping.h"

#include "defaultHud.h"

MBitionMiniHudWarping::MBitionMiniHudWarping(KWin::DefaultHudEffect* hud_effect)
    : QtWaylandServer::mbition_mini_hud_warping_v1()
{
    m_effect = hud_effect;
}

void MBitionMiniHudWarping::mbition_mini_hud_warping_v1_destroy(Resource* resource)
{
    qCInfo(KWINARHUD_DEBUG) << "Destroying mini_hud resource";
    if (!resource)
    {
        qCWarning(KWINARHUD_DEBUG) << "Destroying mini_hud resource failed - resource is nullptr!";
        return;
    }
    wl_resource_destroy(resource->handle);
}

void MBitionMiniHudWarping::mbition_mini_hud_warping_v1_setMatrices([[maybe_unused]] Resource* resource, int fd)
{
    if (!m_effect) {
        qCWarning(KWINARHUD_DEBUG) << "Error - m_effect is nullptr!";
        return;
    }

    m_effect->setMatrices(fd);
}

void MBitionMiniHudWarping::mbition_mini_hud_warping_v1_setMirrorLevel([[maybe_unused]] Resource* resource, wl_fixed_t mirrorLevel)
{
    if (!m_effect) {
        qCWarning(KWINARHUD_DEBUG) << "Error - m_effect is nullptr!";
        return;
    }

    m_effect->setMirrorLevel(static_cast<float>(mirrorLevel));
}

void MBitionMiniHudWarping::mbition_mini_hud_warping_v1_setWhitePoint([[maybe_unused]] Resource* resource, unsigned int red, unsigned int green, unsigned int blue, unsigned int divisor)
{
    if (!m_effect) {
        qCWarning(KWINARHUD_DEBUG) << "Error - m_effect is nullptr!";
        return;
    }

    float div = static_cast<float>(divisor);
    m_effect->setWhitePoint(red / div, green / div, blue / div);
}
