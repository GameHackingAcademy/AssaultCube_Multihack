#pragma once

#include <Windows.h>

class Triggerbot {
private:
	INPUT input = { 0 };
public:
	Triggerbot();
	void execute(int isLookingAtEnemy);
};
