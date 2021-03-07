#pragma once

#include <Windows.h>

DWORD triggerbot_ori_call_address = 0x4607C0;
DWORD triggerbot_ori_jump_address = 0x0040ADA2;

DWORD esp_ret_address = 0x0040BE83;
DWORD text_address = 0x419880;

const char* empty_text = "";
