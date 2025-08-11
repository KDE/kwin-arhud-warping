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

in vec2 multiTexCoord;
out vec2 texCoord;

uniform mat4 params1_x;
uniform mat4 params1_y;
uniform mat4 params2_x;
uniform mat4 params2_y;
uniform mat4 params3_x;
uniform mat4 params3_y;
uniform vec4 uv_span;
uniform float mirrorLevel;
uniform vec2 window_size;

vec2 calculate_position(float level, vec3 row0, vec3 row1) {
    vec3 scales = vec3(0.0, 5.0, 10.0);
    vec3 levels = vec3(level) - scales;

    vec3 localRow0 = 1.0 / (scales - vec3(5.0, 0.0, 0.0));
    vec3 localRow1 = 1.0 / (scales - vec3(10.0, 10.0, 5.0));

    vec3 scale = (levels.yxx * localRow0) * (levels.zzy * localRow1);
    localRow0 = scale * row0;
    localRow1 = scale * row1;

    return vec2(dot(localRow0, vec3(1.0)), dot(localRow1, vec3(1.0)));
}

void main() {
    texCoord = mix(uv_span.xy, uv_span.zw, multiTexCoord);

    // 3.99 => used to convert a number in [0, 1] to [0, 3.99] to be used as indices
    // such that all indices are used proportionately. I.e. [0, ~0.25] => 1, [~0.75, 1] => 3
    ivec2 tex = ivec2(floor(3.99 * multiTexCoord));
    vec3 row0 = vec3(params1_x[tex.x][tex.y], params2_x[tex.x][tex.y], params3_x[tex.x][tex.y]);
    vec3 row1 = vec3(params1_y[tex.x][tex.y], params2_y[tex.x][tex.y], params3_y[tex.x][tex.y]);

    vec2 pos = calculate_position(mirrorLevel, row0, row1);

    vec2 end_pos = vec2(2.0 * pos.x / window_size.x - 1.0, 2.0 * pos.y / window_size.y - 1.0);
    gl_Position = vec4(end_pos, 0.0, 1.0);
}
