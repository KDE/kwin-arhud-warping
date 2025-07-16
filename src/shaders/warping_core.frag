/*
 * SPDX-FileCopyrightText: Copyright (c) 2017-2022, Daimler AG
 * SPDX-FileCopyrightText: Copyright (c) 2022, Mercedes-Benz AG
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#version 300 es

precision highp float;
precision highp int;
precision lowp sampler2D;
precision lowp samplerCube;

uniform sampler2D inputTexture;

in vec2 texCoord0;
out vec4 fragColor;

void main()
{
  //fragColor = vec4(1.0f, 0.0f, 0.0f, 1.0f);
  //fragColor = vec4(texCoord0, 0.0f, 1.0f);
  fragColor = texture(inputTexture, texCoord0);
}

