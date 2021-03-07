#include <Windows.h>

#include "constants.h"
#include "Triggerbot.h"
#include "PlayerGeometry.h"
#include "Menu.h"

Triggerbot *triggerbot;
PlayerGeometry *playerGeometry;
Menu *menu;

HMODULE openGLHandle = NULL;
void(__stdcall* glDepthFunc)(unsigned int) = NULL;
DWORD opengl_ret_address = 0;

DWORD edi_value = 0;

DWORD old_protect;

__declspec(naked) void opengl_codecave() {
	__asm {
		pushad
	}

	if (menu->item_enabled[WALLHACK]) {
		(*glDepthFunc)(0x207);
	}

	// Finally, restore the original instruction and jump back
	__asm {
		popad
		mov esi, dword ptr ds : [esi + 0xA18]
		jmp opengl_ret_address
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

	if (menu->item_enabled[TRIGGERBOT]) {
		triggerbot->execute(edi_value);
	}

	// Restore the registers and jump back to original code
	_asm {
		popad
		jmp triggerbot_ori_jump_address
	}
}

void print_text(DWORD x, DWORD y, const char* text) {
	if (x > 2400 || x < 0 || y < 0 || y > 1800) {
		text = "";
	}

	__asm {
		mov ecx, text
		push y
		push x
		call text_address
		add esp, 8
	}
}

// Our codecave responsible for printing text
__declspec(naked) void text_codecave() {
	// First, recreate the original function we hooked but set the text to empty
	__asm {
		mov ecx, empty_text
		call text_address
		pushad
	}

	for (int i = 0; i < MAX_ITEMS; i++) {
		print_text(50, 250 + (100 * i), menu->items[i]);
		print_text(500, 250 + (100 * i), menu->get_state(i));
	}
	print_text(10, 250 + (100 * menu->cursor_position), menu->cursor);

	if (menu->item_enabled[ESP]) {
		// Next, loop through all the current players in the game
		for (int i = 1; i < *playerGeometry->current_players; i++) {
			print_text(playerGeometry->x_values[i], playerGeometry->y_values[i], playerGeometry->names[i]);
		}
	}

	// Restore the registers and jump back to the original code
	__asm {
		popad
		jmp esp_ret_address
	}
}

// This thread contains all of our aimbot code
void injected_thread() {

	while (true) {
		if (openGLHandle == NULL) {
			openGLHandle = GetModuleHandle(L"opengl32.dll");
		}

		if (openGLHandle != NULL && glDepthFunc == NULL) {
			glDepthFunc = (void(__stdcall*)(unsigned int))GetProcAddress(openGLHandle, "glDepthFunc");

			// Then we find the location of glDrawElements and offset to an instruction that is easy to hook
			unsigned char *opengl_hook_location = (unsigned char*)GetProcAddress(openGLHandle, "glDrawElements");
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

		menu->handle_input();

		playerGeometry->update();

		if (menu->item_enabled[AIMBOT]) {
			playerGeometry->set_player_view();
		}

		// So our thread doesn't constantly run, we have it pause execution for a millisecond.
		// This allows the processor to schedule other tasks.
		Sleep(1);
	}
}

// When our DLL is loaded, create a thread in the process to create the hook
// We need to do this as our DLL might be loaded before OpenGL is loaded by the process
BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved) {
	unsigned char* triggerbot_hook_location = (unsigned char*)0x0040AD9D;
	unsigned char* text_hook_location = (unsigned char*)0x0040BE7E;

	if (fdwReason == DLL_PROCESS_ATTACH) {
		triggerbot = new Triggerbot();
		playerGeometry = new PlayerGeometry(0x509B74, 0x50F4F8, 0x50F500);
		menu = new Menu();

		CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)injected_thread, NULL, 0, NULL);

		VirtualProtect((void*)triggerbot_hook_location, 5, PAGE_EXECUTE_READWRITE, &old_protect);
		*triggerbot_hook_location = 0xE9;
		*(DWORD*)(triggerbot_hook_location + 1) = (DWORD)&triggerbot_codecave - ((DWORD)triggerbot_hook_location + 5);

		VirtualProtect((void*)text_hook_location, 5, PAGE_EXECUTE_READWRITE, &old_protect);
		*text_hook_location = 0xE9;
		*(DWORD*)(text_hook_location + 1) = (DWORD)&text_codecave - ((DWORD)text_hook_location + 5);
	}
	else if (fdwReason == DLL_PROCESS_DETACH) {
		delete triggerbot;
		delete playerGeometry;
		delete menu;
	}

	return true;
}
