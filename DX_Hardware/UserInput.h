#pragma once
class UserInput
{
public:
	UserInput();
	~UserInput();
	char buttons[256]{};
	char buttonbuffer[256]{};
	float x = 0.0f;
	float y = 0.0f;
	float prevX = 0.0f;
	float prevY = 0.0f;
	float diffx = 0.0f;
	float diffy = 0.0f;
	bool mouse_move = false;
	bool mouse_wheel = false;
	bool left_click = false;
	bool right_click = false;
};
