#pragma once

#include "transform.h"
#include "device.h"
#include "camera.h"

void TOS_create_gizmo_context(TOS_device* _device, TOS_camera* _camera);
void TOS_destroy_gizmo_context();

void TOS_set_transform_gizmo_target(TOS_transform* transform);
bool TOS_is_transform_gizmo_active();
void TOS_set_transform_gizmo_op(TOS_transform_op op);

void TOS_tick_transform_gizmo();
void TOS_draw_transform_gizmo();