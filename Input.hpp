#pragma once
#include <cstdint>
#include "Vector.hpp"
#include <windows.h>
#include <functional>

namespace NInput
{
	bool Initiate();
	void Terminate();

	void Update();
	void SetInterruptFunction(std::function<void(UINT uMsg, WPARAM wParam, LPARAM lParam)> && func);

	void BlockInput();
	void UnblockInput();
	bool ToggleInputBlock();

	void LockMouse();
	void UnlockMouse();
	bool ToggleMouseLock();

	NMath::CVec2f GetMousePos();

	bool IsKeyDown(int32_t key);

	bool WasKeyPressed(int32_t key);
	bool WasKeyReleased(int32_t key);

	extern bool input_blocked;
	extern bool mouse_locked;

	extern HWND target_hwnd;
}
