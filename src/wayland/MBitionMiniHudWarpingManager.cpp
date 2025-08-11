// SPDX-FileCopyrightText: Copyright (c) 2025 MBition GmbH.
// SPDX-License-Identifier: GPL-2.0-or-later

#include "MBitionMiniHudWarpingManager.h"

#include "MBitionMiniHudWarping.h"
#include "defaultHud.h"

#include <wayland/display.h>
#include <wayland/output.h>
#include <effect/effecthandler.h>

MBitionMiniHudWarpingManager::MBitionMiniHudWarpingManager(KWin::DefaultHudEffect* effect)
    : QtWaylandServer::mbition_mini_hud_warping_manager_v1(*KWin::effects->waylandDisplay(), 1)
    , m_effect{ effect }
    , m_screen{ nullptr }
{}

void MBitionMiniHudWarpingManager::mbition_mini_hud_warping_manager_v1_get_mini_hud(Resource* resource, uint32_t id,
                                                                                    unsigned int displayWidth, unsigned int displayHeight,
                                                                                    unsigned int appAreaWidth, unsigned int appAreaHeight)
{
    qCInfo(KWINARHUD_DEBUG) << "Incoming request get_mini_hud with id = " << id;
    if (!resource)
    {
        qCWarning(KWINARHUD_DEBUG) << "resource is nullptr!";
        return;
    }
    if (!m_effect)
    {
        qCWarning(KWINARHUD_DEBUG) << "m_effect is nullptr!";
        return;
    }

    qCInfo(KWINARHUD_DEBUG) << "get_mini_hud args:" << displayWidth << displayHeight << appAreaWidth << appAreaHeight;
    m_effect->setHudSize(displayWidth, displayHeight, appAreaWidth, appAreaHeight);

    int screen_index = 0;
    const auto& screens = KWin::effects->screens();
    for (auto screen : screens)
    {
        if (!screen)
        {
            qCWarning(KWINARHUD_DEBUG) << "Printing screen data failed - Screen" << screen_index << "is nullptr!";
            continue;
        }

        qCInfo(KWINARHUD_DEBUG) << "Screen [" << screen_index << "]: " << screen->manufacturer() << "," << screen->model()
                              << "," << screen->name() << "," << screen->geometry();

        if (static_cast<unsigned int>(screen->geometry().width()) == displayWidth &&
            static_cast<unsigned int>(screen->geometry().height()) == displayHeight)
        {
            qCInfo(KWINARHUD_DEBUG) << "Found screen" << screen_index << "with size:" << displayWidth << "x" << displayHeight;
            m_screen = screen;
            break;
        }

        screen_index++;
    }

    if (m_screen != nullptr)
    {
        auto miniHud = m_effect->miniHud(m_screen);
        if (miniHud)
        {
            miniHud->add(resource->client(), id, 1);
            qCInfo(KWINARHUD_DEBUG) << "Screen" << m_screen->name() << m_screen->geometry()
                                  << "is added to the plugin";
        }
        else
        {
            qCWarning(KWINARHUD_DEBUG) << "miniHud is nullptr!";
        }
    }
    else {
        qCWarning(KWINARHUD_DEBUG) << "screen is nullptr!";
    }
}
