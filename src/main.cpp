// SPDX-FileCopyrightText: Copyright (c) 2022 MBition GmbH.
// SPDX-License-Identifier: GPL-2.0-or-later

#include "arhud.h"

namespace KWin
{

KWIN_EFFECT_FACTORY_SUPPORTED(ArHUDEffect,
                              "metadata.json",
                              return ArHUDEffect::supported();)

} // namespace KWin

#include "main.moc"
