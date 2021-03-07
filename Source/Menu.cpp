#include <Windows.h>

#include "Menu.h"

Menu::Menu() {
	cursor_position = 0;
}

void Menu::handle_input() {

	if (GetAsyncKeyState(VK_DOWN) & 1) {
		cursor_position++;
	}
	else if (GetAsyncKeyState(VK_UP) & 1) {
		cursor_position--;
	}
	else if ((GetAsyncKeyState(VK_LEFT) & 1) || (GetAsyncKeyState(VK_RIGHT) & 1)) {
		item_enabled[cursor_position] = !item_enabled[cursor_position];
	}

	if (cursor_position < 0) {
		cursor_position = 3;
	}
	else if (cursor_position > 3) {
		cursor_position = 0;
	}
}

const char* Menu::get_state(int item) {
	return item_enabled[item] ? on_text : off_text;
}
