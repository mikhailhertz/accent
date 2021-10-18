#include <Windows.h>
#include <WtsApi32.h>

typedef struct
{
	COLORREF color1;
	COLORREF color2;
} IMMERSIVE_COLOR_PREFERENCE;

HRESULT(WINAPI * GetUserColorPreference)(IMMERSIVE_COLOR_PREFERENCE * pcpPreference, BOOL fForceReload);

/*
	ActiveCaption = 2
	ActiveCaptionText = 9
	Highlight = 13
	HighlightText = 14
	HotTrack = 26
	GradientActiveCaption = 27
	MenuHighlight = 29
*/

COLORREF get_accent_color()
{
	IMMERSIVE_COLOR_PREFERENCE preference;
	GetUserColorPreference(&preference, 0);
	return preference.color2 & 0x00FFFFFF;
}

HANDLE g_heap;
void set_colors(COLORREF color)
{
	INT elements[] = { COLOR_HIGHLIGHT, COLOR_HOTLIGHT, COLOR_MENUHILIGHT };
	int element_count = sizeof(elements) / sizeof(*elements);
	COLORREF * colors = (COLORREF *)HeapAlloc(g_heap, 0, element_count * sizeof(COLORREF));
	if (colors == NULL)
	{
		return 0;
	}
	for (int i = 0; i < element_count; i++)
	{
		colors[i] = color;
	}
	SetSysColors(element_count, elements, colors);
	HeapFree(g_heap, 0, colors);
}

COLORREF g_current_color;
LRESULT CALLBACK WndProc(HWND window_handle, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
		case WM_DWMCOLORIZATIONCOLORCHANGED:
		{
			COLORREF color  = get_accent_color();
			if (color == g_current_color)
			{
				return 0;
			}
			g_current_color = color;
			set_colors(color);
			break;
		}
		case WM_WTSSESSION_CHANGE:
		{
			set_colors(g_current_color);
			break;
		}
		case WM_CLOSE:
		{
			DestroyWindow(window_handle);
			break;
		}
		case WM_DESTROY:
		{
			PostQuitMessage(0);
			break;
		}
		default:
		{
			return DefWindowProc(window_handle, message, wParam, lParam);
		}
	}
	return 0;
}

int WINAPI WinMain(HINSTANCE program_instance, HINSTANCE unused, LPSTR command_line, int show_command)
{
	g_heap = GetProcessHeap();
	if (g_heap == NULL)
	{
		return 0;
	}
	HMODULE uxtheme_module = LoadLibrary(L"uxtheme.dll");
	if (uxtheme_module == NULL)
	{
		return 0;
	}
	GetUserColorPreference = GetProcAddress(uxtheme_module, "GetUserColorPreference");
	if (GetUserColorPreference == NULL)
	{
		return 0;
	}
	g_current_color = get_accent_color();
	set_colors(g_current_color);
	WNDCLASSEX window_class = { 0 };
	window_class.cbSize = sizeof(WNDCLASSEX);
	window_class.lpfnWndProc = WndProc;
	window_class.hInstance = program_instance;
	window_class.lpszClassName = L"0";
	if (!RegisterClassEx(&window_class))
	{
		return 0;
	}
	HWND window_handle = CreateWindow(L"0", L"", 0, 0, 0, 0, 0, NULL, NULL, program_instance, NULL);
	if (window_handle == NULL)
	{
		return 0;
	}
	WTSRegisterSessionNotification(window_handle, NOTIFY_FOR_THIS_SESSION);
	MSG message;
	while (GetMessage(&message, NULL, 0, 0) > 0)
	{
		DispatchMessage(&message);
	}
	return message.wParam;
}