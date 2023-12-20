#version 330 core

// Copyright (C) 2014 Klaralvdalens Datakonsult AB (KDAB).
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

in vec3 vertexPosition;
in vec3 vertexNormal;

out WorldSpaceVertex {
    vec3 position;
    vec3 normal;
} vs_out;

uniform mat4 modelMatrix;
uniform mat3 modelNormalMatrix;
uniform mat4 mvp;

void main()
{
    vs_out.position = vec3(modelMatrix * vec4(vertexPosition, 1.0));
    vs_out.normal = normalize(modelNormalMatrix * vertexNormal);

    gl_Position = mvp * vec4(vertexPosition, 1.0);
}
