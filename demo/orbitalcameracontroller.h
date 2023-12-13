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
#ifndef QT3DCSG_ORBITALCAMERACONTROLLER_H
#define QT3DCSG_ORBITALCAMERACONTROLLER_H

#include <Qt3DExtras/QOrbitCameraController>

namespace QtCSG::Demo {

// a simple orbital camera controller for inspectiong the scene
// -------------------------------------------------------------------------------------------------
//
// Mouse bindings
//
//  - left button:                          orbits the objects
//  - right button:                         moves the object quickly
//  - right button + shift key:             moves the object slowly
//  - left button + right button:           zooms the objects
//  - left button + alt key:                simulates right button
//
// Keyboard bindings
//  - arrow keys (left, right, up, down):   orbits the object
//  - page up, page down:                   zooms the object
//  - arrow keys plus alt key:              moves the object quickly
//  - arrow keys plus alt key and shift:    moves the object slowly
//
class OrbitCameraController : public Qt3DExtras::QOrbitCameraController
{
public:
    using Qt3DExtras::QOrbitCameraController::QOrbitCameraController;

private:
    void moveCamera(const InputState &state, float dt) override;

    void orbit(float rx, float ry);
    void zoom(float dz);
    void translate(const InputState &state, float dt);
};

} // namespace QtCSG::Demo

#endif // QT3DCSG_ORBITALCAMERACONTROLLER_H
