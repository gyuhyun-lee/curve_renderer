#include <stdio.h>
#include <windows.h>
#include <gl/gl.h>

#include "types.h"
#include "curve.h"
#include "win32.h"

#define STB_TRUETYPE_IMPLEMENTATION
#include "stb_truetype.h"

// NOTE(joon) unity build
#include "curve.cpp"
#include "render.cpp"

// NOTE(joon) global variables, not the prettiest thing to do
global b32 global_is_game_running;

internal void 
win32_resize_DIB_section(Win32OffscreenBuffer* buffer, i32 width, i32 height)
{
    if (buffer->memory)
    {
        VirtualFree(buffer->memory, 0, MEM_RELEASE);
    }

	buffer->memory = 0;
    buffer->width = width;
    buffer->height = height;

    // NOTE: When the biHeight field is negative, this is the clue to
    // Windows to treat this bitmap as top-down, not bottom-up, meaning that
    // the first three bytes of the image are the color for the top left pixel
    // in the bitmap, not the bottom left!
    buffer->info.bmiHeader.biSize = sizeof(buffer->info.bmiHeader);
    buffer->info.bmiHeader.biWidth = buffer->width;
    buffer->info.bmiHeader.biHeight = buffer->height;
    buffer->info.bmiHeader.biPlanes = 1;
    buffer->info.bmiHeader.biBitCount = 32;
    buffer->info.bmiHeader.biCompression = BI_RGB;

    buffer->bytes_per_pixel = 4;

    // now the pitch should be aligned.
    // we don't accept non aligned buffer from now on!
    buffer->pitch = width * buffer->bytes_per_pixel;
    int bitmap_memory_size = buffer->pitch * buffer->height;
    buffer->memory = VirtualAlloc(0, bitmap_memory_size, MEM_COMMIT, PAGE_READWRITE);
}


internal v2 
win32_get_window_dimension(HWND window)
{
	/*
		structure RECT
		left, top, right, bottom
	*/
	RECT clientRect;
	GetClientRect(window, &clientRect);

	v2 result = {};
	result.x = (f32)(clientRect.right - clientRect.left);
	result.y = (f32)(clientRect.bottom - clientRect.top);

	return result;
}

internal void 
win32_display_buffer(HDC device_context)
{
#if 0
	i32 offsetX = 0;
	i32 offsetY = 0;

	if (window_width >= buffer->width * 2 &&
		window_height >= buffer->height * 2)
	{
		StretchDIBits(device_context,
			offsetX, offsetY, 2 * buffer->width, 2 * buffer->height,
			offsetX, offsetY, buffer->width, buffer->height,
			buffer->memory,
			&buffer->info,
			DIB_RGB_COLORS, SRCCOPY);
	}
	else
	{
		PatBlt(device_context, 0, 0, window_width, offsetY, BLACKNESS);
		PatBlt(device_context, 0, offsetY + buffer->height, window_width, window_height, BLACKNESS);
		PatBlt(device_context, 0, 0, offsetX, window_height, BLACKNESS);
		PatBlt(device_context, offsetX + buffer->width, 0, window_width, window_height, BLACKNESS);

		// For prototyping purposes, we're going to always blit 1 to 1 pixels
		StretchDIBits(device_context,
			offsetX, offsetY, buffer->width, buffer->height,
			offsetX, offsetY, buffer->width, buffer->height,
			buffer->memory,
			&buffer->info,
			DIB_RGB_COLORS, SRCCOPY);
	}
#endif


	SwapBuffers(device_context);
}

internal void 
win32_create_menu(HWND window_handle)
{
	HMENU hMenu = CreateMenu();

	// TODO(joon) : checkbox?
	
	// NOTE(joon) 'File' Menu
	HMENU hSubMenu = CreatePopupMenu();
	AppendMenuA(hSubMenu, MF_STRING, menu_item_clear, "Clear");
	AppendMenuA(hSubMenu, MF_STRING, menu_item_exit, "Exit");
	AppendMenuA(hMenu, MF_STRING | MF_POPUP, (UINT)hSubMenu, "Control");

	// NOTE(joon) 'View' Menu
	hSubMenu = CreatePopupMenu();
	AppendMenuA(hSubMenu, MF_STRING, menu_item_show_polyline, "Show Polyline");
	AppendMenuA(hSubMenu, MF_STRING, menu_item_show_point, "Show Points");
	AppendMenuA(hSubMenu, MF_STRING, menu_item_show_shell, "Show Shell");
	AppendMenuA(hMenu, MF_STRING | MF_POPUP, (UINT)hSubMenu, "View");

	hSubMenu = CreatePopupMenu();
	AppendMenuA(hSubMenu, MF_STRING, menu_item_method_nli, "NLI");
	AppendMenuA(hSubMenu, MF_STRING, menu_item_method_bernstein, "Bernstein");
	AppendMenuA(hSubMenu, MF_STRING, menu_item_method_midpoint, "Midpoint");
	AppendMenuA(hMenu, MF_STRING | MF_POPUP, (UINT)hSubMenu, "Select Method");

	// NOTE(joon) 'Method' Menu
	SetMenu(window_handle, hMenu);
}

LRESULT CALLBACK
main_window_proc(HWND window_handle,
	UINT message,
	WPARAM wParam,
	LPARAM lParam)
{
	LRESULT result = 0;

	switch (message)
	{
		case WM_CREATE:
		{
			win32_create_menu(window_handle);
		}break;
		case WM_CLOSE:
		{
			global_is_game_running = false;
		} break;

		case WM_DESTROY:
		{
			global_is_game_running = false;
		} break;

		case WM_SIZE:
		{
		} break;

		case WM_ACTIVATEAPP:

		{
		} break;
		case WM_SYSKEYDOWN:
		case WM_SYSKEYUP:
		case WM_KEYDOWN:
		case WM_KEYUP:
		case WM_LBUTTONDOWN:
		{
			assert("keyboard input should not come in here!!");
		}break;

		case WM_PAINT:
		{
			PAINTSTRUCT paint_struct;
			HDC device_context = BeginPaint(window_handle, &paint_struct);

			win32_display_buffer(device_context);
			EndPaint(window_handle, &paint_struct);
		}break;

		default:
		{
			result = DefWindowProc(window_handle, message, wParam, lParam);
		}break;
	}

	return result;
}

internal v2
win32_get_bottom_up_client_mouse_p(HWND window_handle, f32 buffer_height)
{
	// NOTE(joon) : Get mouse pos
	v2 result = {};

	POINT p = {};
	GetCursorPos(&p);
	ScreenToClient(window_handle, &p);

	result.x = (f32)p.x;
	result.y = buffer_height - (f32)p.y;

	return result;
}

internal void
win32_process_message(Win32State* state, v2 client_rect)
{
	MSG msg;
	while (PeekMessageA(&msg, 0, 0, 0, PM_REMOVE))
	{
		switch (msg.message)
		{
			case WM_LBUTTONDOWN:
			{
				v2 client_mouse_p = win32_get_bottom_up_client_mouse_p(state->window_handle, client_rect.y);
				if (!state->clicked_point && !state->clicked_slider)
				{
					f32 radius_square = state->point_radius * state->point_radius;

					for (u32 point_index = 0;
						point_index < state->point_count;
						++point_index)
					{
						ControlPoint* point = state->points + point_index;
						
						if (distance_square(point->p, client_mouse_p) <= radius_square)
						{
							state->clicked_point = point;
							break;
						}
					}

					v2 cursor_screen_p = hadamard(client_rect, 0.5f * (lerp(state->slider.min, state->slider.cursor, state->slider.max) + V2(1, 1)));
					v2 cursor_screen_half_dim = hadamard(client_rect, state->slider.cursor_half_dim);
					v2 cursor_screen_min = cursor_screen_p - cursor_screen_half_dim;
					v2 cursor_screen_max = cursor_screen_p + cursor_screen_half_dim;
					if (client_mouse_p.x >= cursor_screen_min.x && client_mouse_p.y >= cursor_screen_min.y &&
						client_mouse_p.x < cursor_screen_max.x && client_mouse_p.y < cursor_screen_max.y)
					{
						state->clicked_slider = &state->slider;
					}
				}

	#if 0
				char buffer[256];
				sprintf_s(buffer, "mouse left down\n");
				OutputDebugStringA(buffer);
	#endif

				state->is_mouse_left_down = true;
			}break;
			case WM_LBUTTONUP:
			{
				v2 client_mouse_p = win32_get_bottom_up_client_mouse_p(state->window_handle, client_rect.y);
				if (!state->clicked_point && !state->clicked_slider)
				{
					ControlPoint* point = state->points + state->point_count++;
					point->p = client_mouse_p;

					assert(state->point_count <= 20);
				}

				// cleanup
				state->is_mouse_left_down = false;
				state->clicked_point = 0;
				state->clicked_slider = 0;
			}break;

			// NOTE(joon) windows menu control
			case WM_COMMAND:
			{
				int menu_id = LOWORD(msg.wParam);
				switch (menu_id)
				{
					case menu_item_clear:
					{
						for (u32 point_index = 0;
							point_index < state->point_count;
							++point_index)
						{
							state->points[point_index].p = {};
						}

						state->point_count = 0;
					}break;
					case menu_item_exit:
					{
						global_is_game_running = false;
					}break;

					case menu_item_show_polyline:
					{
						state->show_polyline = !state->show_polyline;
					}break;
					case menu_item_show_point:
					{
						state->show_point = !state->show_point;
					}break;
					case menu_item_show_shell:
					{
						state->show_shell = !state->show_shell;
					}break;

					case menu_item_method_nli:
					{
						state->method = curve_method_nli;
					}break;
					case menu_item_method_bernstein:
					{
						state->method = curve_method_bernstein;
					}break;
					case menu_item_method_midpoint:
					{
						state->method = curve_method_midpoint;
					}break;
				}
			}break;
		}

		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
}


internal void
win32_init_gl(HWND window)
{
	HDC device_context = GetDC(window);

	PIXELFORMATDESCRIPTOR desired_pixel_format = {};

	desired_pixel_format.nSize = sizeof(PIXELFORMATDESCRIPTOR);
	desired_pixel_format.nVersion = 1;
	desired_pixel_format.dwFlags = PFD_SUPPORT_OPENGL|PFD_DRAW_TO_WINDOW|PFD_DOUBLEBUFFER;
	desired_pixel_format.iLayerType = PFD_MAIN_PLANE;
	desired_pixel_format.cColorBits = 32;
	desired_pixel_format.cAlphaBits = 8;
	//cDepthBits

	int suggested_pixel_format_index = ChoosePixelFormat(device_context, &desired_pixel_format);
	PIXELFORMATDESCRIPTOR suggested_pixel_format = {};
	DescribePixelFormat(device_context, suggested_pixel_format_index, sizeof(suggested_pixel_format), &suggested_pixel_format);
	SetPixelFormat(device_context, suggested_pixel_format_index, &suggested_pixel_format);

	HGLRC gl_rc = wglCreateContext(device_context);
	if (wglMakeCurrent(device_context, gl_rc))
	{
		// success!
	}
	else
	{
		//TODO(joon) Can this even fail?
		invalid_code_path;
	}

	ReleaseDC(window, device_context);
}

struct win32_read_entire_file_result
{
	void* memory;
	u32 size;
};

internal win32_read_entire_file_result
win32_read_entire_file(const char *file_name)
{
	win32_read_entire_file_result result = {};

	HANDLE file_handle = CreateFileA(file_name, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, 0, 0);

	if (file_handle != INVALID_HANDLE_VALUE)
	{
		LARGE_INTEGER file_size = {};
		/*
		Using GetFileSizeEx instead of GetFileSize because GetFileSize can only hold 32 bit value
		- which means up to 4 gigabytes.
		*/
		if (GetFileSizeEx(file_handle, &file_size))
		{
			u32 file_size_32 = (u32)file_size.QuadPart;
			result.memory = VirtualAlloc(0, file_size_32, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
			if (result.memory)
			{
				DWORD bytes_read;

				if (ReadFile(file_handle, result.memory, file_size_32, &bytes_read, 0) &&
					file_size_32 == bytes_read)
				{
					result.size = bytes_read;
				}
				else
				{
					free(result.memory);
					result.memory = 0;
					result.size = 0;
				}
			}
			else
			{
				invalid_code_path;
			}
		}
		else
		{
			invalid_code_path;
		}

		CloseHandle(file_handle);
	}
	else
	{
		invalid_code_path;
	}

	return result;
}

internal Win32Slider
win32_make_horizontal_slider(v2 min, v2 max, v2 cursor_half_dim)
{
	Win32Slider result = {};
	result.cursor_half_dim = cursor_half_dim;
	result.min = min;
	result.max = max;
	result.cursor = 0.5f;

	return result;
}

int CALLBACK
WinMain(HINSTANCE hInstance,
	HINSTANCE HPrevInstance,
	LPSTR lpCmdLine,
	int nCmdShow)
{
#if 0
	// TODO : Make this happen!
	int32 screen_width = 1920;
	int32 screen_height = 1080;
#else
	i32 screen_width = 1280;
	i32 screen_height = 720;
#endif

#if PROJECTH_DEBUG
	globalDEBUGShowCursor = true;
#endif

	/*Make window class*/
	WNDCLASSEXA window_class = {};
	window_class.cbSize = sizeof(window_class); //size of this structure
	window_class.style = CS_HREDRAW | CS_VREDRAW;
	window_class.lpfnWndProc = main_window_proc; //Function Pointer to the Windows Procedure function
	window_class.hInstance = hInstance;
	window_class.lpszClassName = "windowclass";
	window_class.hCursor = LoadCursor(0, IDC_ARROW);

	if (RegisterClassExA(&window_class))
	{
		HMENU dropdown_menu_handle = CreatePopupMenu();

		MENUITEMINFOA menu_item_info = {};
		menu_item_info.cbSize = sizeof(MENUITEMINFOA);
		menu_item_info.fMask = MIIM_ID;
		menu_item_info.fType = MFT_MENUBARBREAK;
		menu_item_info.fState = MFS_DEFAULT;
		menu_item_info.wID = 10000;
		//menu_item_info.dwTypeData = "asdfasdfasdf";

		int identifier = 9001;
		InsertMenuItemA(dropdown_menu_handle, identifier, FALSE, &menu_item_info);
		
		HWND window_handle =
			CreateWindowExA(
				0,
				window_class.lpszClassName,
				"1111",
				WS_OVERLAPPEDWINDOW | WS_VISIBLE,
				CW_USEDEFAULT,
				CW_USEDEFAULT,
				screen_width,
				screen_height,
				0,
				dropdown_menu_handle,
				hInstance,
				0);

		if (window_handle)
		{
			win32_init_gl(window_handle);
			v2 client_rect = win32_get_window_dimension(window_handle);
			f32 client_width_over_height = client_rect.x/client_rect.y;
			Win32State state = {};
			state.window_handle = window_handle;
			state.point_radius = 10;
			state.show_point = true;
			state.show_polyline = true;
			state.show_shell = true;
			state.slider = win32_make_horizontal_slider({ -0.95f, 0.7f }, { -0.6f, 0.7f }, { 0.01f, 0.02f * client_width_over_height});

			HDC refreshDC = GetDC(window_handle);
			int monitorRefreshHz = 60;
			int win32RefreshRate = GetDeviceCaps(refreshDC, VREFRESH);
			ReleaseDC(window_handle, refreshDC);
			if (win32RefreshRate > 1)
			{
				monitorRefreshHz = win32RefreshRate;
			}
#if 1
			// 30fps
			int gameUpdateHz = monitorRefreshHz / 2;
#else
			// 60fps
			int gameUpdateHz = monitorRefreshHz;
#endif
			f32 targetSecondsPerFrame = 1.0f / (f32)60;

			u32 memory_arena_size = 1024 * 1024 * 1024;
			void* memory_arena_memory = VirtualAlloc(0, memory_arena_size, MEM_COMMIT, PAGE_READWRITE);
			MemoryArena arena = start_memory_arena(memory_arena_memory, memory_arena_size);

			i32 font_bitmap_width = 512;
			i32 font_bitmap_height = 512;
			u8 *font_bitmap = (u8*)malloc(sizeof(u8) * font_bitmap_width * font_bitmap_height);

			u32 glyph_count = 256; // only needs ascii
			stbtt_bakedchar *glyph_infos = (stbtt_bakedchar *)malloc(sizeof(stbtt_bakedchar) * glyph_count);
			win32_read_entire_file_result font = win32_read_entire_file("C:/Windows/Fonts/ARIALN.ttf");

			int result = stbtt_BakeFontBitmap((unsigned char*)font.memory, 0,
											  50.0f, // TODO(joon) This does not correspond to the actual pixel size, but to get higher pixel density, we need to crank this up
											  (unsigned char*)font_bitmap, font_bitmap_width, font_bitmap_height,
											  0, glyph_count,
											  glyph_infos);

			u32 *font_texture = (u32 *)malloc(sizeof(u32) * font_bitmap_width * font_bitmap_height);

			u8* source = font_bitmap;
			u32* dest = font_texture;
			for (i32 y = 0;
				y < font_bitmap_height;
				++y)
			{
				for (i32 x = 0;
					x < font_bitmap_width;
					++x)
				{
					if (*source != 0)
					{
						//v3 color = {1, 1, 1};
						//u32 color_u32 = get_color_from_v3(( 1.0f - (*source / 255.0f)) * color);
						//*dest = color_u32;
						// TODO(joon) These colors are hack!
						*dest = 0xff000000;
					}
					else
					{
						*dest = 0xffffffff;
					}

					dest++;
					source++;
				}
			}

#if 1
			GLuint font_texture_handle = 0;
			glGenTextures(1, &font_texture_handle);

			glBindTexture(GL_TEXTURE_2D, font_texture_handle);
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, font_bitmap_width, font_bitmap_height, 1, GL_BGRA_EXT, GL_UNSIGNED_BYTE, font_texture);

			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
			glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);// GL_MODULATE?
			glBindTexture(GL_TEXTURE_2D, 0);
#endif
			
			v2 clip_font_half_dim = { 0.015f, 0.015f * client_width_over_height };
#if 0
			// NOTE(joon) not supported
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_LOD, 0);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LOD, 0);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_R, GL_CLAMP);
#endif

			global_is_game_running = true;
			while (global_is_game_running)
			{
				win32_process_message(&state, client_rect);

				v2 client_mouse_p = win32_get_bottom_up_client_mouse_p(window_handle, client_rect.y);
				if (state.clicked_point)
				{
					state.clicked_point->p = client_mouse_p;
				}
				if (state.clicked_slider)
				{
					v2 clip_mouse_p = {};
					clip_mouse_p.x = 2.0f * ((client_mouse_p.x / client_rect.x) - 0.5f);
					f32 a = (clip_mouse_p.x - state.clicked_slider->min.x) / (state.clicked_slider->max.x - state.clicked_slider->min.x);
					state.clicked_slider->cursor = clamp(0.0f, a, 1.0f);
				}

				// NOTE(joon) rendering
				glViewport(0, 0, (GLsizei)client_rect.x, (GLsizei)client_rect.y);
				glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
				glClear(GL_COLOR_BUFFER_BIT);
				glMatrixMode(GL_MODELVIEW);
				glLoadIdentity();
				glMatrixMode(GL_PROJECTION);
				glLoadIdentity();
				glMatrixMode(GL_TEXTURE);
				glLoadIdentity();
				glDisable(GL_TEXTURE_2D);

				if(state.show_polyline)
				{
					gl_render_polylines(state.points, state.point_count, client_rect, {1.0f, 0.0f, 0.0f});
				}

				char method_buffer[256] = {};

				f32 t_step_size = 0.005f;
				v3 curve_color = {};
				// NOTE(joon) draw curves 
				switch (state.method)
				{
					case curve_method_nli:
					{
						gl_render_nli_v2(state.points, state.point_count, t_step_size, client_rect, curve_color);

						if (state.show_shell)
						{
							gl_render_nli_shell(state.points, state.point_count, state.slider.cursor, client_rect, {0, 0, 1});
						}
						sprintf_s(method_buffer, 256, "%s", "NLI");
					}break;

					case curve_method_bernstein:
					{
						gl_render_bernstein_v2(state.points, state.point_count, t_step_size, client_rect, curve_color);
						sprintf_s(method_buffer, 256, "%s", "BERNSTEIN");
					}break;

					case curve_method_midpoint:
					{
						gl_render_midpoint_v2(&arena, state.points, state.point_count, client_rect, curve_color);
						sprintf_s(method_buffer, 256, "%s", "MIDPOINT");
					}break;
				}

				if (state.show_point)
				{
					// NOTE(joon) draw points
					gl_render_points_in_quads(state.points, state.point_count, client_rect, 
											{ state.point_radius, state.point_radius }, {0, 0.0f, 0.0f});
				}

				// NOTE(joon) draw slider for t in NLI
				glBegin(GL_QUADS);
				{
					v2 cursor = lerp(state.slider.min, state.slider.cursor, state.slider.max);
					v2 cursor_min = cursor - state.slider.cursor_half_dim;
					v2 cursor_max = cursor + state.slider.cursor_half_dim;

					glVertex2f(cursor_min.x, cursor_min.y);
					glVertex2f(cursor_max.x, cursor_min.y);
					glVertex2f(cursor_max.x, cursor_max.y);
					glVertex2f(cursor_min.x, cursor_max.y);
				}glEnd();
				glBegin(GL_LINES);
				{
					glColor3f(0, 0, 0);
					glVertex2f(state.slider.min.x, state.slider.min.y);
					glVertex2f(state.slider.max.x, state.slider.max.y);
				}glEnd();

				char buffer[256] = {};
				sprintf_s(buffer, 256, "DEGREE %d\nMETHOD %s", state.point_count + 1, method_buffer);
				gl_render_characters_in_line(buffer, {-0.98f, 0.95f}, clip_font_half_dim, font_texture_handle, glyph_infos, font_bitmap_width, font_bitmap_height);

				gl_render_characters_in_line("T", state.slider.min + V2(-0.015f, 0), clip_font_half_dim, font_texture_handle, glyph_infos, font_bitmap_width, font_bitmap_height);

				// TODO(joon) hack to fallback memory arena 
				arena.used = 0;

				// NOTE(joon): Display
				HDC device_context = GetDC(window_handle);
				win32_display_buffer(device_context);
				ReleaseDC(window_handle, device_context);
			}
		}
		else
		{
			// NOTE(joon) creating window failed.
			assert(0);
		}
	}
	else
	{
		//Registering Windows Class Failed.
		assert(0);
	}

	return 0;
}