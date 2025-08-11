// SPDX-FileCopyrightText: Copyright (c) 2025 MBition GmbH.
// SPDX-License-Identifier: GPL-2.0-or-later

#include "MBitionWarpedOutputManager.h"

#include "MBitionWarpedOutput.h"
#include "classicArHud.h"

#include <wayland/display.h>
#include <wayland/output.h>
#include <effect/effecthandler.h>

MBitionWarpedOutputManager::MBitionWarpedOutputManager(KWin::ClassicArHudEffect* effect)
    : QtWaylandServer::zmbition_warped_output_manager_v1(*KWin::effects->waylandDisplay(), 1)
    , m_effect(effect)
{}

void MBitionWarpedOutputManager::zmbition_warped_output_manager_v1_get_warped_output(Resource* resource,
                                                                                     uint32_t  id,
                                                                                     struct ::wl_resource* /*output*/)
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
            warpedOutput->add(resource->client(), id, 1);
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
