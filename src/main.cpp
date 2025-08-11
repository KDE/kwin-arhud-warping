// SPDX-FileCopyrightText: Copyright (c) 2025 MBition GmbH.
// SPDX-License-Identifier: GPL-2.0-or-later

#include "warpingEffect.h"

namespace KWin
{

KWIN_EFFECT_FACTORY_SUPPORTED(WarpingEffect,
                                "metadata.json",
                                return WarpingEffect::supported();)

} // namespace KWin

#include "main.moc"
