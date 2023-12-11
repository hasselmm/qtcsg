/* QtCSG provides Constructive Solid Geometry (CSG) for Qt
 * Copyright Ⓒ 2023 Mathias Hasselmann
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
#include "orbitalcameracontroller.h"

#include <Qt3DRender/QCamera>

namespace QtCSG::Demo {

void OrbitCameraController::moveCamera(const InputState &state, float dt)
{
    if (state.rightMouseButtonActive && state.leftMouseButtonActive) {
        zoom(state.ryAxisValue * dt);
    } else if (state.rightMouseButtonActive
               || (state.leftMouseButtonActive && state.altKeyActive)) {
        if (state.shiftKeyActive) {
            translate(state, dt);
        } else {
            translate(state, dt * 2.5);
        }
    } else if (state.leftMouseButtonActive) {
        orbit(state.rxAxisValue * dt, state.ryAxisValue * dt);
    }

    if (state.altKeyActive) {
        if (state.shiftKeyActive) {
            translate(state, dt / 2.5);
        } else {
            translate(state, dt);
        }
    } else {
        orbit(state.txAxisValue * dt, state.tyAxisValue * dt);
        zoom(state.tzAxisValue * dt);
    }
}

void OrbitCameraController::orbit(float rx, float ry)
{
    camera()->panAboutViewCenter(rx * lookSpeed(), {0, 1, 0});
    camera()->tiltAboutViewCenter(ry * lookSpeed());
}

void OrbitCameraController::zoom(float dz)
{
    const auto camera = this->camera();
    const auto zoomDistance = (camera->viewCenter() - camera->position()).lengthSquared();

    if (zoomDistance > zoomInLimit() * zoomInLimit()) { // Dolly up to limit
        camera->translate({0, 0, linearSpeed() * dz}, camera->DontTranslateViewCenter);
    } else {
        camera->translate({0, 0, -0.5}, camera->DontTranslateViewCenter);
    }
}

void OrbitCameraController::translate(const InputState &state, float dt)
{
    const auto dx = qBound<float>(-1, state.rxAxisValue + state.txAxisValue, 1);
    const auto dy = qBound<float>(-1, state.ryAxisValue + state.tyAxisValue, 1);
    camera()->translate({dx * linearSpeed() * dt, dy * linearSpeed() * dt, 0});
}

} // namespace QtCSG::Demo
