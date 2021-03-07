#pragma once

#include <Windows.h>

#define M_PI 3.14159265358979323846
#define MAX_PLAYERS 32

// The player structure for every player in the game
struct Player {
	char unknown1[4];
	float x;
	float y;
	float z;
	char unknown2[0x30];
	float yaw;
	float pitch;
	char unknown3[0x1DD];
	char name[16];
	char unknown4[0x103];
	int dead;
};

class PlayerGeometry {
private:
	DWORD player_offset_address;
	DWORD enemy_list_address;
	DWORD current_players_address;

	float closest_yaw;
	float closest_pitch;

	Player* player;

	float euclidean_distance(float, float);
public:
	DWORD x_values[MAX_PLAYERS] = { 0 };
	DWORD y_values[MAX_PLAYERS] = { 0 };
	char* names[MAX_PLAYERS] = { NULL };

	int* current_players;

	PlayerGeometry(DWORD, DWORD, DWORD);
	void update();
	void set_player_view();
};
