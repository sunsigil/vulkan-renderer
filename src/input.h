#pragma once

#include "context.h"
#include "glm/glm.hpp"

void TOS_create_input_context(TOS_context* context);
void TOS_input_tick();

bool TOS_key_down(int key);
bool TOS_key_up(int key);
bool TOS_key_pressed(int key);
bool TOS_key_released(int key);

glm::vec2 TOS_mouse_position();
glm::vec2 TOS_mouse_delta();
bool TOS_mouse_down();
bool TOS_mouse_up();
bool TOS_mouse_pressed();
bool TOS_mouse_released();

void TOS_toggle_cursor(bool state);