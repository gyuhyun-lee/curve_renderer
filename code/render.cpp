// get the value that ranges from minus one to one
inline v2
get_clip_p(v2 p, v2 dim)
{
	v2 result = {};
	result.x = (p.x / dim.x) * 2.0f - 1.0f;
	result.y = (p.y / dim.y) * 2.0f - 1.0f;

	return result;
}

internal void
clear_buffer(Win32OffscreenBuffer* buffer, v3 color)
{
	u32 color_u32 = get_color_from_v3(color);
	u8 *row = (u8*)buffer->memory;
	for (i32 y = 0;
		y < buffer->height;
		++y)
	{
		u32* pixel = (u32*)row;
		for (i32 x = 0;
			x < buffer->width;
			++x)
		{
			*pixel++ = color_u32;
		}

		row += buffer->pitch;
	}
}

internal void
render_point_circle(Win32OffscreenBuffer *buffer, v2 pixel_p, f32 radius, v3 color)
{
	v2 origin = pixel_p;
	f32 radius_square = (f32)(radius * radius);
	u32 color_u32 = get_color_from_v3(color);

	i32 min_x = round_f32_i32(pixel_p.x - radius);
	i32 max_x = round_f32_i32(pixel_p.x + radius);

	i32 min_y = round_f32_i32(pixel_p.y - radius);
	i32 max_y = round_f32_i32(pixel_p.y + radius);
	
	if (min_x < 0)
	{
		min_x = 0;
	}
	if (min_x >= buffer->width)
	{
		min_x = buffer->width-1;
	}

	if (max_x < 0)
	{
		max_x = 0;
	}
	if (max_x >= buffer->width)
	{
		max_x = buffer->width-1;
	}

	if (min_y < 0)
	{
		min_y = 0;
	}
	if (min_y >= buffer->height)
	{
		min_y = buffer->height-1;
	}

	if (max_y < 0)
	{
		max_y = 0;
	}
	if (max_y >= buffer->height)
	{
		max_y = buffer->height-1;
	}

	u8 *row = (u8*)buffer->memory + min_y * buffer->pitch + min_x * buffer->bytes_per_pixel;
	for (i32 y = min_y;
		y < max_y;
		++y)
	{
		u32* pixel = (u32*)row;
		for (i32 x = min_x;
			x < max_x;
			++x)
		{
			v2 p = {(f32)x, (f32)y};
			if (distance_square(origin, p) <= radius_square)
			{
				*pixel = color_u32;
			}
			pixel++;
		}

		row += buffer->pitch;
	}
}


// NOTE(joon) uses Bresenham line drawing algorithm
internal void
render_line(Win32OffscreenBuffer* buffer,
			v2 start, v2 end, v3 color)
{
	u32 color_u32 = get_color_from_v3(color);

	i32 start_x = round_f32_i32(start.x);
	i32 end_x = round_f32_i32(end.x);

	i32 start_y = round_f32_i32(start.y);
	i32 end_y = round_f32_i32(end.y);

	if (start_x < 0)
	{
		start_x = 0;
	}
	if (start_x >= buffer->width)
	{
		start_x = buffer->width-1;
	}

	if (end_x < 0)
	{
		end_x = 0;
	}
	if (end_x >= buffer->width)
	{
		end_x = buffer->width-1;
	}

	if (start_y < 0)
	{
		start_y = 0;
	}
	if (start_y >= buffer->height)
	{
		start_y = buffer->height-1;
	}

	if (end_y < 0)
	{
		end_y = 0;
	}
	if (end_y >= buffer->height)
	{
		end_y = buffer->height-1;
	}

	int dx = end_x - start_x;
	int dy = end_y - start_y;
	int abs_dx = abs(dx);
	int abs_dy = abs(dy);
	int sign_x = dx > 0 ? 1 : -1;
	int sign_y = dy > 0 ? 1 : -1;

	int x = start_x;
	int y = start_y;

	u32 *pixel = (u32 *)buffer->memory + y * buffer->width + x;

	if (dx != 0 && dy != 0)
	{
		*pixel = color_u32; // draw the starting pixel
		
		if (abs_dx > abs_dy)
		{
			// NOTE(joon) slope is smaller than 1
			int pk = (2 * abs_dy) - abs_dx;
			for (i32 i = 0; 
					 i < abs_dx; 
					 i++)
			{
				x = x + sign_x;
				pixel += sign_x;

				if (pk < 0)
				{
					pk = pk + (2 * abs_dy);
				}
				else
				{
					y = y + sign_y;
					pixel += sign_y * buffer->width;
					pk = pk + (2 * abs_dy) - (2 * abs_dx);
				}

				*pixel = color_u32; // draw the starting pixel
			}
		}
		else
		{
			// NOTE(joon) slope is bigger than or equal to 1
			int pk = (2 * abs_dx) - abs_dy;
			for (i32 i = 0; 
					 i < abs_dy; 
					 i++)
			{
				y = y + sign_y;
				pixel += sign_y * buffer->width;

				if (pk < 0)
				{
					pk = pk + (2 * abs_dx);
				}
				else
				{
					x = x + sign_x;
					pixel += sign_x;
					pk = pk + (2 * abs_dx) - (2 * abs_dy);
				}

				*pixel = color_u32; // draw the starting pixel
			}
		}
	}
}

internal void
gl_render_points_in_quads(ControlPoint *points, i32 point_count, v2 client_rect, v2 half_dim, v3 color)
{
	v2 clip_half_dim = {};
	clip_half_dim.x = half_dim.x / client_rect.x;
	clip_half_dim.y = half_dim.y / client_rect.y;

	glBegin(GL_QUADS);
	{
		glColor3f(color.r, color.g, color.b);
		// NOTE(joon) draw points
		for (i32 point_index = 0;
			point_index < point_count;
			++point_index)
		{
			ControlPoint* point = points + point_index;
			v2 clip_p = get_clip_p(point->p, client_rect);
			v2 min_clip_p = clip_p - clip_half_dim;
			v2 max_clip_p = clip_p + clip_half_dim;

			glVertex2f(min_clip_p.x, min_clip_p.y);
			glVertex2f(max_clip_p.x, min_clip_p.y);
			glVertex2f(max_clip_p.x, max_clip_p.y);
			glVertex2f(min_clip_p.x, max_clip_p.y);
		}
	}glEnd();
}

internal void
gl_render_polylines(ControlPoint *points, i32 point_count, v2 client_rect, v3 color)
{
	glBegin(GL_LINE_STRIP);
	{
		glColor3f(color.r, color.g, color.b);

		for (i32 point_index = 0;
			point_index < point_count;
			++point_index)
		{
			ControlPoint* point = points + point_index;
			v2 clip_p = get_clip_p(point->p, client_rect);

			glVertex2f(clip_p.x, clip_p.y);
		}

	}glEnd();
}

 /*
	TODO(joon)
	2. implement windows menu for dropdown menu
	4. more efficient NLI
	5. finish the project...

	//3. more efficient factorial for bernstein
 */
internal void 
gl_render_bernstein_v2(ControlPoint *points, i32 point_count, f32 t_step_size, v2 client_rect, v3 color)
{
	i32 degree = point_count - 1;
	long long fact_degree = factorial(degree);

	glBegin(GL_LINE_STRIP);
	{
		glColor3f(color.r, color.g, color.b);
		v2 first_clip_p = get_clip_p(points[0].p, client_rect);
		glVertex2f(first_clip_p.x, first_clip_p.y);


		for (f32 t = 0.0f;
			t < 1.0f;
			t += t_step_size)
		{
			v2 next_point = {};

			for (i32 i = 0;
				i <= degree;
				++i)
			{
				f32 binomial = (f32)((fact_degree / (factorial(degree - i) * factorial(i))));
				f32 pow_one_minus_t = powf(1.0f - t, (f32)(degree - i));
				f32 pow_t = powf(t, (f32)i);

				f32 bernstein_value = binomial * pow_one_minus_t * pow_t;

				next_point.x += bernstein_value * points[i].p.x;
				next_point.y += bernstein_value * points[i].p.y;
			}
			
			v2 next_clip_p = get_clip_p(next_point, client_rect);
			glVertex2f(next_clip_p.x, next_clip_p.y);
		}
	}glEnd();
}

// TODO(joon) make this faster!
internal v2
nli(ControlPoint *points, int degree, int i, float t)
{
	if (degree <= 0)
	{
		return points[i].p;
	}

	return (1.0f - t) * nli(points, degree - 1, i, t) + t * nli(points, degree - 1, i + 1, t);
}

internal void 
gl_render_nli_v2(ControlPoint *points, i32 point_count, f32 t_step_size, v2 client_rect, v3 color)
{
	i32 degree = point_count - 1;

	glBegin(GL_LINE_STRIP);
	{
		glColor3f(color.r, color.g, color.b);
		v2 first_clip_p = get_clip_p(points[0].p, client_rect);
		glVertex2f(first_clip_p.x, first_clip_p.y);

		for (f32 t = 0.0f;
			t < 1.0f;
			t += t_step_size)
		{
			v2 next_clip_p = get_clip_p(nli(points, degree, 0, t), client_rect);
			glVertex2f(next_clip_p.x, next_clip_p.y);
		}
	}glEnd();
}

// TODO(joon) make this faster!
internal v2
nli_shell(ControlPoint *points, int degree, int i, float t, v2 client_rect)
{
	if (degree <= 0)
	{
		return points[i].p;
	}

	v2 p0 = nli_shell(points, degree - 1, i, t, client_rect);
	v2 p1 = nli_shell(points, degree - 1, i + 1, t, client_rect);

	if (degree > 1)
	{
		v2 clip_p0 = get_clip_p(p0, client_rect);
		v2 clip_p1 = get_clip_p(p1, client_rect);
		
		v2 half_dim = { 0.005f, 0.005f * (client_rect.x / client_rect.y) };
		glBegin(GL_QUADS);
		{
			glVertex2f(clip_p0.x-half_dim.x, clip_p0.y-half_dim.y);
			glVertex2f(clip_p0.x+half_dim.x, clip_p0.y-half_dim.y);
			glVertex2f(clip_p0.x+half_dim.x, clip_p0.y+half_dim.y);
			glVertex2f(clip_p0.x-half_dim.x, clip_p0.y+half_dim.y);

			glVertex2f(clip_p1.x-half_dim.x, clip_p1.y-half_dim.y);
			glVertex2f(clip_p1.x+half_dim.x, clip_p1.y-half_dim.y);
			glVertex2f(clip_p1.x+half_dim.x, clip_p1.y+half_dim.y);
			glVertex2f(clip_p1.x-half_dim.x, clip_p1.y+half_dim.y);
		}glEnd();

		glBegin(GL_LINES);
		{
			glVertex2f(clip_p0.x, clip_p0.y);
			glVertex2f(clip_p1.x, clip_p1.y);
		}glEnd();
	}

	return (1.0f - t) * p0 + t * p1;
}


internal void
gl_render_nli_shell(ControlPoint* points, i32 point_count, f32 t, v2 client_rect, v3 color)
{
	i32 degree = point_count - 1;
	glColor3f(color.r, color.g, color.b);

	nli_shell(points, degree, 0, t, client_rect);
}

internal void
midpoint(MemoryArena* arena, v2 *points, i32 point_count, i32 iter, v2 client_rect)
{
	if (iter == 4)
	{
		// draw
		for (i32 point_index = 0;
			point_index < point_count;
			++point_index)
		{
			v2 clip_p = get_clip_p(points[point_index], client_rect);

			glVertex2f(clip_p.x, clip_p.y);
		}
		return;
	}

	i32 midpoints_to_generate_count = gauss_sum_exclude(1, point_count);
	v2* midpoints = push_array(arena, v2, (i32)midpoints_to_generate_count);
	// IMPORTANT(joon) points & midpoints should be sequential!!!!
	assert(points + point_count == midpoints);

	// first column will generate point_count - amount of midpoints
	// this value will decrement by 1 when we advance per column
	i32 midpoint_to_calculate_for_this_column_count = point_count - 1;
	
	v2 *source = points;
	v2 *dest = midpoints;
	while (midpoint_to_calculate_for_this_column_count != 0)
	{
		for (i32 midpoint_index = 0;
			midpoint_index < midpoint_to_calculate_for_this_column_count;
			++midpoint_index)
		{
			*dest = 0.5f * (*source + *(source + 1));

			source++;
			dest++;
		}

		midpoint_to_calculate_for_this_column_count--;
		source++;
	}

	v2 *next_points_0 = push_array(arena, v2, point_count);
	i32 stride = 0;
	for (i32 next_point_index = 0;
		next_point_index < point_count;
		++next_point_index)
	{
		next_points_0[next_point_index] = points[stride];

		stride += point_count - next_point_index;
	}
	midpoint(arena, next_points_0, point_count, iter + 1, client_rect);

	v2 *next_points_1 = push_array(arena, v2, point_count);
	stride = gauss_sum(1, point_count) - 1;
	for (i32 next_point_index = 0;
		next_point_index < point_count;
		++next_point_index)
	{
		next_points_1[next_point_index] = points[stride];

		stride -= (next_point_index+1);
	}
	midpoint(arena, next_points_1, point_count, iter + 1, client_rect);
}

internal void 
gl_render_midpoint_v2(MemoryArena *arena, ControlPoint *control_points, i32 point_count, v2 client_rect, v3 color)
{
	if (point_count > 1)
	{
		glBegin(GL_LINE_STRIP);
		{
			glColor3f(color.r, color.g, color.b);

			v2* points = push_array(arena, v2, point_count);
			for (i32 point_index = 0;
				point_index < point_count;
				++point_index)
			{
				points[point_index] = control_points[point_index].p;
			}

			midpoint(arena, points, point_count, 0, client_rect);

		}glEnd();
	}
}

internal void
calculate_g_in_newton_form(f64* g_x, f64 *g_y, u32 input_start_index, u32 input_count, u32 denominator)
{
	u32 one_past_input_end_index = input_start_index + input_count;
	u32 output_index = one_past_input_end_index;

	for (u32 input_index = input_start_index;
		input_index < one_past_input_end_index - 1;
		++input_index)
	{
		g_x[output_index] = (g_x[input_index + 1] - g_x[input_index]) / denominator;
		g_y[output_index] = (g_y[input_index + 1] - g_y[input_index]) / denominator;

		output_index++;
	}
}

internal f64
get_newton_form_value(f64* g, u32 g_count, u32 point_count, f64 t)
{
	f64 result = 0.0;

	u32 input_stride = point_count;
	u32 t_count = 0;
	for (u32 g_index = 0;
		g_index < g_count;
		)
	{
		f64 coefficient = g[g_index];

		for (u32 t_index = 0;
			t_index < t_count;
			++t_index)
		{
			coefficient *= (t - (f64)t_index);
		}

		result += coefficient; // first value of the newton form

		g_index += input_stride;
		input_stride--;
		t_count++;
	}

	return result;
}

internal void
gl_render_newton_form(MemoryArena* arena, ControlPoint* control_points, i32 point_count, v2 client_rect, v3 color)
{
	if (point_count > 1)
	{
		glBegin(GL_LINE_STRIP);
		{
			glColor3f(color.r, color.g, color.b);

			u32 g_count = gauss_sum(1, point_count);
			// TODO(joon) precision issue, use other type?
			f64* g_x = push_array(arena, f64, g_count);
			f64* g_y = push_array(arena, f64, g_count);

			// Copy the input values that are needed to calculate g[]
			for (u32 point_index = 0;
				point_index < (u32)point_count;
				++point_index)
			{
				g_x[point_index] = control_points[point_index].p.x;
				g_y[point_index] = control_points[point_index].p.y;
			}

			u32 input_count = point_count;
			u32 g_denominator = 1;
			for (u32 input_start_index = 0;
				input_start_index < g_count;
				)
			{
				calculate_g_in_newton_form(g_x, g_y, input_start_index, input_count, g_denominator);

				g_denominator++;
				input_start_index += input_count;
				input_count--;
			}

			for (f64 t = 0.0f;
				t <= (f64)(point_count - 1);
				t += 0.0001)
			{
				f64 x = get_newton_form_value(g_x, g_count, point_count, t);
				f64 y = get_newton_form_value(g_y, g_count, point_count, t);

				v2 client_p = V2((f32)x, (f32)y);
				v2 clip_p = get_clip_p(client_p, client_rect);

				glVertex2f(clip_p.x, clip_p.y);
			}

		}glEnd();
	}
}

internal void
gl_render_characters_in_line(const char *characters, 
							v2 start_p, v2 half_dim, 
							GLuint font_texture_ID, stbtt_bakedchar *glyph_infos, 
							i32 font_texture_width, i32 font_texture_height)
{
	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, font_texture_ID);

	glBegin(GL_TRIANGLES);
	{
		glColor3f(1, 1, 1);

		const char* start = characters;
		v2 p = start_p;
		while (*start != '\0')
		{
			switch (*start)
			{
				case ' ':
				{
					p.x += 2.0f * half_dim.x;
				}break;
				case '\n':
				{
					p.x = start_p.x;
					p.y -= 2.3f * half_dim.y;
				}break;

				default:
				{
					stbtt_bakedchar* glyph_info = glyph_infos + *start;

					f32 u0 = ((f32)glyph_info->x0 / (f32)font_texture_width);
					f32 u1 = ((f32)glyph_info->x1 / (f32)font_texture_width);
					// NOTE(joon) These are flipped, because the texture we have is top_down
					f32 v1 = ((f32)glyph_info->y0 / (f32)font_texture_height);
					f32 v0 = ((f32)glyph_info->y1 / (f32)font_texture_height);

					v2 min_corner = p - half_dim;
					v2 max_corner = p + half_dim;

					glTexCoord2f(u0, v0);
					glVertex2f(min_corner.x, min_corner.y);

					glTexCoord2f(u1, v0);
					glVertex2f(max_corner.x, min_corner.y);

					glTexCoord2f(u1, v1);
					glVertex2f(max_corner.x, max_corner.y);

#if 1
					glTexCoord2f(u0, v0);
					glVertex2f(min_corner.x, min_corner.y);

					glTexCoord2f(u1, v1);
					glVertex2f(max_corner.x, max_corner.y);

					glTexCoord2f(u0, v1);
					glVertex2f(min_corner.x, max_corner.y);

					p.x += 2.0f * half_dim.x;
				};
			}

			start++;
		}
#endif
	}glEnd();
}
















