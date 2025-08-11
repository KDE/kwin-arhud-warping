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

in vec2 position;
in vec2 texcoord;
out vec2 texCoord0;

//uniform mat4 qt_Matrix;
uniform mat4 modelViewProjectionMatrix;
uniform sampler2D warpingMatrixTexture;
uniform int matrixCount;
uniform vec2 matrixResolution;
uniform float matrixInterpolationFactor;
uniform int matrixInterpolationIndex;
uniform vec4 uvFunc;

float ConvertToFloat(vec4 v)
{
  uvec4 uv = clamp(uvec4(round(v * 255.0f)), 0U, 255U);
  uint u = uv.x + (uv.y << 8) + (uv.z << 16) + (uv.w << 24);
  float f = float(u) / float(0xffffffffU); // At this point we lose some precision.
  return f * 4.0f - 2.0f;
}

vec2 GetVertexSSPos(int mIndex)
{
  float cu = float(matrixResolution.x) * 2.0f;
  float cv = float(matrixResolution.y);

  float u0 =  (texcoord.x * (cu - 2.0f) + 0.5f) / cu;
  float v  = ((texcoord.y * (cv - 1.0f) + 0.5f) / cv + float(mIndex)) / float(matrixCount);

  float u1 = u0 + 1.0f / cu;

  return vec2(ConvertToFloat(textureLod(warpingMatrixTexture, vec2(u0, v), 0.0f).xyzw),
     ConvertToFloat(textureLod(warpingMatrixTexture, vec2(u1, v), 0.0f).xyzw));
}

void main()
{
  vec2 ssPos = mix(
  GetVertexSSPos(matrixInterpolationIndex),
  GetVertexSSPos(matrixInterpolationIndex + 1),
  matrixInterpolationFactor);

  texCoord0 = texcoord * uvFunc.xy + uvFunc.zw;

  gl_Position = vec4(ssPos.x, -ssPos.y, 0.0f, 1.0f);
}

