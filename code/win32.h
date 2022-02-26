#ifndef WIN32_H
#define WIN32_H

enum Win32MenuItemID
{
	// Inside 'File' Menu
	menu_item_clear = 1001,
	menu_item_exit,

	// Inside 'View' Menu
	menu_item_show_polyline,
	menu_item_show_point,
	menu_item_show_shell,

	// Inside 'Method' Menu
	menu_item_method_nli,
	menu_item_method_bernstein,
	menu_item_method_midpoint,
	menu_item_method_newton,
};

struct Win32OffscreenBuffer
{
	i32 width;
	i32 height;
	i32 pitch;

	i32 bytes_per_pixel;
	BITMAPINFO info;

	void* memory;
};

struct Win32WindowDimension
{
	i32 width;
	i32 height;
};

struct Win32Slider
{
	f32 cursor; // linear interpolation between min and max 

	v2 cursor_half_dim;

	v2 min;
	v2 max;
};

struct Win32State
{
	HWND window_handle;

	ControlPoint points[20];
	u32 point_count;
	f32 point_radius;
	ControlPoint* clicked_point;

	Win32Slider slider;
	Win32Slider *clicked_slider;

	CurveMethod method;

	b32 is_mouse_left_down;
	b32 show_polyline;
	b32 show_point;
	b32 show_shell;
};

#endif