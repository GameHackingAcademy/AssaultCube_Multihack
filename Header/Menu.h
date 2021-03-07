#pragma once

#define WALLHACK 0
#define ESP 1
#define AIMBOT 2
#define TRIGGERBOT 3

#define MAX_ITEMS 4

class Menu {
private:
	const char on_text[5] = { 0xc, 0x30, 'O', 'N', 0 };
	const char off_text[6] = { 0xc, 0x33, 'O', 'F', 'F', 0 };
public:
	int cursor_position;

	const char* items[MAX_ITEMS] = { "Wallhack", "ESP", "Aimbot", "Triggerbot" };
	bool item_enabled[MAX_ITEMS] = { false };

	const char* cursor = ">";

	Menu();
	void handle_input();
	const char* get_state(int);
};
