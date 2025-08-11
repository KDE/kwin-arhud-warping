// SPDX-FileCopyrightText: Copyright (c) 2025 MBition GmbH.
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <effect/effect.h>

#include <memory>

namespace KWin
{

class EffectBase;

class ClassicArHudEffect;
class DefaultHudEffect;

class WarpingEffect : public Effect
{
    Q_OBJECT
public:
    WarpingEffect();
    ~WarpingEffect() override;

    void paintScreen(const RenderTarget& renderTarget, const RenderViewport& viewport, int mask, const QRegion& region, Output* screen) override;
    bool isActive() const override;
    static bool supported();

private:
    const std::unique_ptr<ClassicArHudEffect> m_arHudEffect;
    const std::unique_ptr<DefaultHudEffect> m_miniArHudEffect;
};

}
