// SPDX-FileCopyrightText: Copyright (c) 2025 MBition GmbH.
// SPDX-License-Identifier: GPL-2.0-or-later

#include "warpingEffect.h"
#include "defaultHud.h"
#include "classicArHud.h"
#include "kwinarhud_debug.h"

#include <effect/effecthandler.h>

namespace KWin {

WarpingEffect::WarpingEffect()
    : m_arHudEffect(std::make_unique<ClassicArHudEffect>())
    , m_miniArHudEffect(std::make_unique<DefaultHudEffect>())
{}

WarpingEffect::~WarpingEffect() = default;

void WarpingEffect::paintScreen(const RenderTarget& renderTarget, const RenderViewport& viewport, int mask, const QRegion& region, Output* screen) {
    if (m_arHudEffect->isActive()) {
        m_arHudEffect->paintScreen(renderTarget, viewport, mask, region, screen);
    } else {
        m_miniArHudEffect->paintScreen(renderTarget, viewport, mask, region, screen);
    }
}

bool WarpingEffect::isActive() const {
    if (m_arHudEffect && m_miniArHudEffect) {
        return m_arHudEffect->isActive() || m_miniArHudEffect->isActive();
    }
    return false;
}

bool WarpingEffect::supported() {
    return effects->compositingType() == OpenGLCompositing && effects->waylandDisplay();
}

}
