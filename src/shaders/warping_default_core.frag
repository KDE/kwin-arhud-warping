/*
 * SPDX-FileCopyrightText: Copyright (c) 2017-2022, Daimler AG
 * SPDX-FileCopyrightText: Copyright (c) 2022-2025, Mercedes-Benz AG
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#version 300 es

precision highp float;
precision highp int;
precision lowp sampler2D;
precision lowp samplerCube;

in vec2 texCoord;
out vec4 fragColor;

uniform sampler2D source;
uniform vec3 whitePointCorrection;

void main() {
    vec3 diffuse = whitePointCorrection * texture(source, texCoord).rgb;
    fragColor = vec4(diffuse.rgb, 1.0);
}
