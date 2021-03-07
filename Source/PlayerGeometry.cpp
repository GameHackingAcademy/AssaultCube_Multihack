#include <Windows.h>
#include <math.h>

#include "PlayerGeometry.h"

PlayerGeometry::PlayerGeometry(DWORD p_address, DWORD e_address, DWORD cp_address) {
	player_offset_address = p_address;
	enemy_list_address = e_address;
	current_players_address = cp_address;
}

// Function to calculate the euclidean distance between two points
float PlayerGeometry::euclidean_distance(float x, float y) {
	return sqrtf((x * x) + (y * y));
}

void PlayerGeometry::update() {
	// First, grab the current position and view angles of our player
	DWORD* player_offset = (DWORD*)(player_offset_address);
	player = (Player*)(*player_offset);

	// Then, get the current number of players in the game
	current_players = (int*)(0x50F500);

	float closest_player = -1.0f;
	closest_yaw = 0.0f;
	closest_pitch = 0.0f;

	// Iterate through all active enemies
	for (int i = 0; i < *current_players; i++) {
		DWORD* enemy_list = (DWORD*)(0x50F4F8);
		DWORD* enemy_offset = (DWORD*)(*enemy_list + (i * 4));
		Player* enemy = (Player*)(*enemy_offset);

		// Make sure the enemy is valid and alive
		if (player != NULL && enemy != NULL) {
			// Calculate the absolute position of the enemy away from us to ensure that our future calculations are correct and based
			// around the origin
			float abspos_x = enemy->x - player->x;
			float abspos_y = enemy->y - player->y;
			float abspos_z = enemy->z - player->z;

			// Calculate our distance from the enemy
			float temp_distance = euclidean_distance(abspos_x, abspos_y);
			// If this is the closest enemy so far, calculate the yaw and pitch to aim at them

			float azimuth_xy = atan2f(abspos_y, abspos_x);
			float yaw = (float)(azimuth_xy * (180.0 / M_PI));
			yaw += 90;

			// Calculate the difference between our current yaw and the calculated yaw to the enemy
			float yaw_dif = player->yaw - yaw;

			// If we are near the 275 angle boundary, our yaw_dif will be too large, causing our text to appear incorrectly
			// To compensate for that, subtract the yaw_dif from 360 if it is over 180, since our viewport can never show 180 degrees
			if (yaw_dif > 180) {
				yaw_dif = yaw_dif - 360;
			}

			if (yaw_dif < -180) {
				yaw_dif = yaw_dif + 360;
			}

			// Calculate our X value by adding the yaw_dif times a scaling factor to the center of the screen horizontally (1200)
			x_values[i] = (DWORD)(1200 + (yaw_dif * -30));

			// Calculate the pitch
			// Since Z values are so limited, pick the larger between x and y to ensure that we 
			// don't look straight at the air when close to an enemy
			if (abspos_y < 0) {
				abspos_y *= -1;
			}
			if (abspos_y < 5) {
				if (abspos_x < 0) {
					abspos_x *= -1;
				}
				abspos_y = abspos_x;
			}
			float azimuth_z = atan2f(abspos_z, abspos_y);
			float pitch = (float)(azimuth_z * (180.0 / M_PI));
			// Same as above but for pitch
			float pitch_dif = player->pitch - pitch;

			// Calculate our Y value by adding the pitch_dif times a scaling factor to the center of the screen vertically (900)
			y_values[i] = (DWORD)(900 + ((pitch_dif) * 25));

			// Set the name to the enemy name
			names[i] = enemy->name;

			if ((closest_player == -1.0f || temp_distance < closest_player) && !enemy->dead) {
				closest_player = temp_distance;
				closest_yaw = yaw;
				closest_pitch = pitch;
			}
		}
	}
}

void PlayerGeometry::set_player_view() {
	player->yaw = closest_yaw;
	player->pitch = closest_pitch;
}
