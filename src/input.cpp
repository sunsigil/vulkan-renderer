#include "input.h"

#include <map>

static TOS_context* context;

static std::map<int, std::pair<bool, bool>> key_mask;

static std::map<int, std::pair<bool, bool>> mouse_mask;
static glm::vec2 mouse_position;
static glm::vec2 mouse_delta;

static bool cursor;

void TOS_create_input_context(TOS_context* _context)
{
	context = _context;

	key_mask = std::map<int, std::pair<bool, bool>>();
	for(int i = 0; i < GLFW_KEY_LAST; i++)
		key_mask[i] = std::pair<bool, bool>(false, false);

	mouse_mask = std::map<int, std::pair<bool, bool>>();
	for(int i = 0; i < GLFW_MOUSE_BUTTON_LAST; i++)
		mouse_mask[i] = std::pair<bool, bool>(false, false);
	mouse_position = glm::vec2(context->window_width/2, context->window_height/2);
	mouse_delta = glm::vec2(0, 0);
	glfwSetCursorPos(context->window_handle, mouse_position.x, mouse_position.y);
}

void TOS_toggle_cursor(bool state)
{
	cursor = state;
	glfwSetInputMode(context->window_handle, GLFW_CURSOR, cursor ? GLFW_CURSOR_NORMAL : GLFW_CURSOR_DISABLED);
}

void TOS_input_tick()
{
	for(int i = 0; i < GLFW_KEY_LAST; i++)
	{
		key_mask[i].second = key_mask[i].first;
		key_mask[i].first = glfwGetKey(context->window_handle, i);
	}

	for(int i = 0; i < GLFW_MOUSE_BUTTON_LAST; i++)
	{
		mouse_mask[i].second = mouse_mask[i].first;
		mouse_mask[i].first = glfwGetMouseButton(context->window_handle, i);
	}

	double x, y;
	glfwGetCursorPos(context->window_handle, &x, &y);
	glm::vec2 new_mouse_position = glm::vec2(x, y);
	mouse_delta = new_mouse_position - mouse_position;
	if(!cursor)
	{
		new_mouse_position = glm::vec2(context->window_width/2, context->window_height/2);
		glfwSetCursorPos(context->window_handle, new_mouse_position.x, new_mouse_position.y);
	}
	mouse_position = new_mouse_position;
}

bool TOS_key_down(int key)
{
	return key_mask[key].first;
}

bool TOS_key_up(int key)
{
	return !key_mask[key].first;
}

bool TOS_key_pressed(int key)
{
	return key_mask[key].first && !key_mask[key].second;
}

bool TOS_key_released(int key)
{
	return !key_mask[key].first && key_mask[key].second;
}

glm::vec2 TOS_mouse_position(bool normalize)
{
	if(normalize)
	{
		return glm::vec2
		(
			mouse_position.x / context->window_width,
			mouse_position.y / context->window_height
		);
	}
	return mouse_position;
}

glm::vec2 TOS_mouse_delta(bool normalize)
{
	if(normalize)
	{
		return glm::vec2
		(
			mouse_delta.x / context->window_width,
			mouse_delta.y / context->window_height
		);
	}
	return mouse_delta;
}

bool TOS_mouse_down()
{
	return mouse_mask[GLFW_MOUSE_BUTTON_1].first;
}

bool TOS_mouse_up()
{
	return !mouse_mask[GLFW_MOUSE_BUTTON_1].first;
}

bool TOS_mouse_pressed()
{
	return mouse_mask[GLFW_MOUSE_BUTTON_1].first && !mouse_mask[GLFW_MOUSE_BUTTON_1].second;
}

bool TOS_mouse_released()
{
	return !mouse_mask[GLFW_MOUSE_BUTTON_1].first && mouse_mask[GLFW_MOUSE_BUTTON_1].second;
}