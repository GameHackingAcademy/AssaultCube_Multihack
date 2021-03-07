#include <Windows.h>
#include <math.h>

#include "Triggerbot.h"

#define M_PI 3.14159265358979323846
#define MAX_PLAYERS 32

Triggerbot *triggerbot;

HMODULE openGLHandle = NULL;
void(__stdcall* glDepthFunc)(unsigned int) = NULL;
void(__stdcall* glDisable)(unsigned int) = NULL;
unsigned char* opengl_hook_location;
DWORD opengl_ret_address = 0;

DWORD triggerbot_ori_call_address = 0x4607C0;
DWORD triggerbot_ori_jump_address = 0x0040ADA2;
DWORD edi_value = 0;

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

// Our player
Player* player = NULL;

DWORD esp_ret_address = 0x0040BE83;
DWORD text_address = 0x419880;

// Our temporary variables for our print text codecave
const char* text = "";
const char* empty_text = "";

DWORD x = 0;
DWORD y = 0;

// List of calculated ESP values
DWORD x_values[MAX_PLAYERS] = { 0 };
DWORD y_values[MAX_PLAYERS] = { 0 };
char* names[MAX_PLAYERS] = { NULL };

int* current_players;

DWORD old_protect;

// Function to calculate the euclidean distance between two points
float euclidean_distance(float x, float y) {
	return sqrtf((x * x) + (y * y));
}

__declspec(naked) void opengl_codecave() {
	__asm {
		pushad
	}

	(*glDepthFunc)(0x207);

	// Finally, restore the original instruction and jump back
	__asm {
		popad
		mov esi, dword ptr ds : [esi + 0xA18]
		jmp opengl_ret_address
	}
}

// The injected thread responsible for creating our hooks
void opengl_thread() {
	while (true) {
		// Since OpenGL will be loaded dynamically into the process, our thread needs to wait
		// until it sees that the OpenGL module has been loaded.
		if (openGLHandle == NULL) {
			openGLHandle = GetModuleHandle(L"opengl32.dll");
		}

		if (openGLHandle != NULL && glDepthFunc == NULL) {
			glDepthFunc = (void(__stdcall*)(unsigned int))GetProcAddress(openGLHandle, "glDepthFunc");
			glDisable = (void(__stdcall*)(unsigned int))GetProcAddress(openGLHandle, "glDisable");

			// Then we find the location of glDrawElements and offset to an instruction that is easy to hook
			opengl_hook_location = (unsigned char*)GetProcAddress(openGLHandle, "glDrawElements");
			opengl_hook_location += 0x16;

			// For the hook, we unprotect the memory at the code we wish to write at
			// Then set the first opcode to E9, or jump
			// Caculate the location using the formula: new_location - original_location+5
			// And finally, since the first original instructions totalled 6 bytes, NOP out the last remaining byte
			VirtualProtect((void*)opengl_hook_location, 5, PAGE_EXECUTE_READWRITE, &old_protect);
			*opengl_hook_location = 0xE9;
			*(DWORD*)(opengl_hook_location + 1) = (DWORD)&opengl_codecave - ((DWORD)opengl_hook_location + 5);
			*(opengl_hook_location + 5) = 0x90;

			// Since OpenGL is loaded dynamically, we need to dynamically calculate the return address
			opengl_ret_address = (DWORD)(opengl_hook_location + 0x6);
		}
		else {
			break;
		}

		// So our thread doesn't constantly run, we have it pause execution for a millisecond.
		// This allows the processor to schedule other tasks.
		Sleep(1);
	}
}

// Our codecave that program execution will jump to. The declspec naked attribute tells the compiler to not add any function
// headers around the assembled code
__declspec(naked) void triggerbot_codecave() {
	// Asm blocks allow you to write pure assembly
	// In this case, we use it to call the function we hooked and save all the registers
	// After we make the call, we move its return value in eax into a variable
	__asm {
		call triggerbot_ori_call_address
		pushad
		mov edi_value, eax
	}

	triggerbot->execute(edi_value);

	// Restore the registers and jump back to original code
	_asm {
		popad
		jmp triggerbot_ori_jump_address
	}
}

// Our codecave responsible for printing text
__declspec(naked) void esp_codecave() {
	current_players = (int*)(0x50F500);

	// First, recreate the original function we hooked but set the text to empty
	__asm {
		mov ecx, empty_text
		call text_address
		pushad
	}

	// Next, loop through all the current players in the game
	for (int i = 1; i < *current_players; i++) {
		// Store the calculated screen positions in temporary variables
		x = x_values[i];
		y = y_values[i];
		text = names[i];

		// Make sure our text is on screen
		if (x > 2400 || x < 0 || y < 0 || y > 1800) {
			text = "";
		}

		// Invoke the print text function to display the text
		__asm {
			mov ecx, text
			push y
			push x
			call text_address
			add esp, 8
		}
	}

	// Restore the registers and jump back to the original code
	__asm {
		popad
		jmp esp_ret_address
	}
}

// This thread contains all of our aimbot code
void aimbot_thread() {

	while (true) {
		// First, grab the current position and view angles of our player
		DWORD* player_offset = (DWORD*)(0x509B74);
		player = (Player*)(*player_offset);

		// Then, get the current number of players in the game
		int* current_players = (int*)(0x50F500);

		// These variables will be used to hold the closest enemy to us
		float closest_player = -1.0f;
		float closest_yaw = 0.0f;
		float closest_pitch = 0.0f;

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

		// When our loop ends, set our yaw and pitch to the closst values
		player->yaw = closest_yaw;
		player->pitch = closest_pitch;

		// So our thread doesn't constantly run, we have it pause execution for a millisecond.
		// This allows the processor to schedule other tasks.
		Sleep(1);
	}
}

// When our DLL is loaded, create a thread in the process to create the hook
// We need to do this as our DLL might be loaded before OpenGL is loaded by the process
BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved) {
	unsigned char* triggerbot_hook_location = (unsigned char*)0x0040AD9D;
	unsigned char* esp_hook_location = (unsigned char*)0x0040BE7E;

	if (fdwReason == DLL_PROCESS_ATTACH) {
		triggerbot = new Triggerbot();

		CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)opengl_thread, NULL, 0, NULL);
		CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)aimbot_thread, NULL, 0, NULL);

		VirtualProtect((void*)triggerbot_hook_location, 5, PAGE_EXECUTE_READWRITE, &old_protect);
		*triggerbot_hook_location = 0xE9;
		*(DWORD*)(triggerbot_hook_location + 1) = (DWORD)&triggerbot_codecave - ((DWORD)triggerbot_hook_location + 5);

		VirtualProtect((void*)esp_hook_location, 5, PAGE_EXECUTE_READWRITE, &old_protect);
		*esp_hook_location = 0xE9;
		*(DWORD*)(esp_hook_location + 1) = (DWORD)&esp_codecave - ((DWORD)esp_hook_location + 5);
	}
	else if (fdwReason == DLL_PROCESS_DETACH) {
		delete triggerbot;
	}

	return true;
}
