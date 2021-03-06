#include <Windows.h>

#include "Triggerbot.h"

Triggerbot::Triggerbot() {
	input = { 0 };
}

void Triggerbot::execute(int isLookingAtEnemy) {
	// If the result of the call is not zero, then we are looking at a player
	// Create a mouse event to simulate the left mouse button being pressed down and send it to the game
	// Otherwise, raise the mouse button up so we stop firing
	if (isLookingAtEnemy != 0) {
		input.type = INPUT_MOUSE;
		input.mi.dwFlags = MOUSEEVENTF_LEFTDOWN;
		SendInput(1, &input, sizeof(INPUT));
	}
	else {
		input.type = INPUT_MOUSE;
		input.mi.dwFlags = MOUSEEVENTF_LEFTUP;
		SendInput(1, &input, sizeof(INPUT));
	}
}
