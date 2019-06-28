#include "Input.hpp"

#include <set>
#include <map>
#include <Windows.h>
#include <MinHook.h>

namespace
{
	struct KeyInfo
	{
		bool key_states[2] = { false, false };
	};

	HWND _target_hwnd = nullptr;
	std::function<void(UINT uMsg, WPARAM wParam, LPARAM lParam)> interrupt_func = nullptr;


	bool data_switch = true;

	std::map<int32_t, KeyInfo> key_state;
	std::set<int32_t> updated_keys;

	WNDPROC wndProc_Old;

	LRESULT CALLBACK newWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) 
	{
		if (NInput::input_blocked)
		{
			switch (uMsg)
			{
			case WM_KEYDOWN:
				//case WM_KEYUP:
			case WM_CAPTURECHANGED:
			case WM_LBUTTONDBLCLK:
			case WM_LBUTTONDOWN:
			case WM_MBUTTONDBLCLK:
			case WM_MBUTTONDOWN:
			case WM_MBUTTONUP:
			case WM_MOUSEACTIVATE:
			case WM_MOUSEHOVER:
			case WM_MOUSEHWHEEL:
			case WM_MOUSELEAVE:
			case WM_MOUSEMOVE:
			case WM_MOUSEWHEEL:
			case WM_NCHITTEST:
			case WM_NCLBUTTONDBLCLK:
			case WM_NCLBUTTONDOWN:
			case WM_NCLBUTTONUP:
			case WM_NCMBUTTONDBLCLK:
			case WM_NCMBUTTONDOWN:
			case WM_NCMBUTTONUP:
			case WM_NCMOUSEHOVER:
			case WM_NCMOUSELEAVE:
			case WM_NCMOUSEMOVE:
			case WM_NCRBUTTONDBLCLK:
			case WM_NCRBUTTONDOWN:
			case WM_NCRBUTTONUP:
			case WM_NCXBUTTONDBLCLK:
			case WM_NCXBUTTONDOWN:
			case WM_NCXBUTTONUP:
			case WM_RBUTTONDBLCLK:
			case WM_RBUTTONDOWN:
			case WM_RBUTTONUP:
			case WM_XBUTTONDBLCLK:
			case WM_XBUTTONDOWN:
			case WM_XBUTTONUP:
			case WM_LBUTTONUP:
				return 1L;
			default:
				;
			}
		}
		else
		{
			if (interrupt_func)
				interrupt_func(uMsg, wParam, lParam);
		}

		return CallWindowProc(wndProc_Old, hwnd, uMsg, wParam, lParam);
	}

	using tSetCursorPos = BOOL(WINAPI *) (int X, int Y);

	// Pointer for calling original MessageBoxW.
	tSetCursorPos oSetCursorPos = nullptr;

	// Detour function which overrides MessageBoxW.
	BOOL WINAPI DetourSetCursorPos(int X, int Y)
	{
		if (!NInput::mouse_locked)
			return true;

		return oSetCursorPos(X, Y);
	}
}


namespace NInput
{
	bool input_blocked = false;
	bool mouse_locked = true;

	bool Initiate(HWND target_hwnd)
	{
		if (MH_CreateHook(&SetCursorPos, &DetourSetCursorPos,
			reinterpret_cast<LPVOID*>(&oSetCursorPos)) != MH_OK)
		{
			return false;
		}

		if (MH_EnableHook(&SetCursorPos) != MH_OK)
		{
			return false;
		}

		_target_hwnd = target_hwnd;

		return wndProc_Old = (WNDPROC)SetWindowLongPtr(target_hwnd, GWL_WNDPROC, (LONG)newWndProc);
	}

	void Terminate()
	{
		SetWindowLongPtr(_target_hwnd, GWL_WNDPROC, (LONG)wndProc_Old);
	}

	void Update()
	{
		data_switch = !data_switch;
		updated_keys.clear();
	}

	void SetInterruptFunction(std::function<void(UINT uMsg, WPARAM wParam, LPARAM lParam)> && func)
	{
		interrupt_func = std::forward<std::function<void(UINT uMsg, WPARAM wParam, LPARAM lParam)>>(func);
	}

	void BlockInput()
	{
		input_blocked = true;
	}

	void UnblockInput()
	{
		input_blocked = false;
	}

	bool ToggleInputBlock()
	{
		return input_blocked = !input_blocked;
	}

	void LockMouse()
	{
		mouse_locked = true;
	}

	void UnlockMouse()
	{
		mouse_locked = false;
	}

	bool ToggleMouseLock()
	{
		return mouse_locked = !mouse_locked;
	}

	NMath::CVec2f GetMousePos()
	{
		POINT temp;
		GetCursorPos(&temp);
		ScreenToClient(_target_hwnd, &temp);

		return NMath::CVec2f(temp.x, temp.y);
	}

	bool IsKeyDown(int32_t key)
	{
		if (GetForegroundWindow() != _target_hwnd)
			return false;

		if (updated_keys.find(key) != updated_keys.end())
			return key_state[key].key_states[static_cast<uint8_t>(data_switch)];

		bool key_held;

		if (input_blocked)
		{
			input_blocked = false;
			{
				key_held = static_cast<bool>(GetKeyState(key) & 0x8000);
			}
			input_blocked = true;
		}
		else
		{
			key_held = static_cast<bool>(GetKeyState(key) & 0x8000);
		}

		key_state[key].key_states[static_cast<uint8_t>(data_switch)] = key_held;
		updated_keys.insert(key);

		return key_held;
	}

	bool WasKeyPressed(int32_t key)
	{
		if (GetForegroundWindow() != _target_hwnd)
			return false;

		if (updated_keys.find(key) != updated_keys.end())
		{
			return key_state[key].key_states[static_cast<uint8_t>(data_switch)] && !key_state[key].key_states[static_cast<uint8_t>(!data_switch)];
		}

		bool key_held;

		if (input_blocked)
		{
			input_blocked = false;
			{
				key_held = static_cast<bool>(GetKeyState(key) & 0x8000);
			}
			input_blocked = true;
		}
		else
		{
			key_held = static_cast<bool>(GetKeyState(key) & 0x8000);
		}

		key_state[key].key_states[static_cast<uint8_t>(data_switch)] = key_held;

		updated_keys.insert(key);

		return key_held && !key_state[key].key_states[static_cast<uint8_t>(!data_switch)];
	}

	bool WasKeyReleased(int32_t key)
	{
		if (GetForegroundWindow() != _target_hwnd)
			return false;

		if (updated_keys.find(key) != updated_keys.end())
		{
			return !key_state[key].key_states[static_cast<uint8_t>(data_switch)] && key_state[key].key_states[static_cast<uint8_t>(!data_switch)];
		}

		bool key_held;

		if (input_blocked)
		{
			input_blocked = false;
			{
				key_held = static_cast<bool>(GetKeyState(key) & 0x8000);
			}
			input_blocked = true;
		}
		else
		{
			key_held = static_cast<bool>(GetKeyState(key) & 0x8000);
		}

		key_state[key].key_states[static_cast<uint8_t>(data_switch)] = key_held;

		updated_keys.insert(key);

		return !key_held && key_state[key].key_states[static_cast<uint8_t>(!data_switch)];
	}
}
