#ifndef TYPES_H
#define TYPES_H

#include <stdint.h>
#include <math.h>

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef u32 b32;

typedef int8_t i8;
typedef int16_t i16;
typedef int32_t i32;

typedef float f32;
typedef double f64;

#define local static
#define global static
#define internal static

#define assert(expression) if(!(expression)) {int *a = 0; *a = 0;}
#define invalid_code_path assert(0)

struct MemoryArena
{
	void* base;
	u32 used;
	u32 total_size;
};

// NOTE(joon): Works for both platform memory(world arena) & temp memory
#define push_array(memory, type, count) (type *)push_size(memory, count * sizeof(type))
#define push_struct(memory, type) (type *)push_size(memory, sizeof(type))

internal MemoryArena
start_memory_arena(void* base, size_t size)
{
	MemoryArena result = {};

	result.base = base;
	result.total_size = (u32)size;

	return result;
}

internal void*
push_size(MemoryArena* memory_arena, size_t size)
{
	assert(size != 0);
	assert(memory_arena->used <= memory_arena->total_size);

	void* result = (u8*)memory_arena->base + memory_arena->used;
	memory_arena->used += (u32)size;

	return result;
}


struct v2
{
	union
	{
		struct
		{
			f32 x;
			f32 y;
		};

		f32 e[2];
	};
};

inline v2
V2(f32 x, f32 y)
{
	v2 result = { x, y };
	return result;
}

inline v2
operator *(f32 value, v2 a)
{
	v2 result = {};

	result.x = a.x * value;
	result.y = a.y * value;

	return result;
}

inline v2
operator +(v2 a, v2 b)
{
	v2 result = {};
	result.x = a.x + b.x;
	result.y = a.y + b.y;

	return result;
}

inline v2
operator -(v2 a, v2 b)
{
	v2 result = {};
	result.x = a.x - b.x;
	result.y = a.y - b.y;

	return result;
}

inline v2
hadamard(v2 a, v2 b)
{
	v2 result = {};
	result.x = a.x * b.x;
	result.y = a.y * b.y;

	return result;
}

inline v2
lerp(v2 min, f32 t, v2 max)
{
	v2 result = (1.0f - t) * min + t * max;

	return result;
}

struct v3
{
	union
	{
		struct
		{
			f32 x;
			f32 y;
			f32 z;
		};

		struct
		{
			f32 r;
			f32 g;
			f32 b;
		};

		f32 e[3];
	};
};

inline v3
operator *(f32 value, v3 a)
{
	v3 result = {};

	result.x = a.x * value;
	result.y = a.y * value;
	result.z = a.z * value;

	return result;
}

// NOTE(joon) ex) 1, 5 gives 1+2+3+4
inline i32
gauss_sum_exclude(i32 start, i32 one_past_end)
{
	i32 result = 0;
	for (i32 i = start;
		i < one_past_end;
		++i)
	{
		result += i;
	}

	return result;
}

// NOTE(joon) ex) 1, 5 gives 1+2+3+4+5
inline i32
gauss_sum(i32 start, i32 end)
{
	i32 result = 0;
	for (i32 i = start;
		i <= end;
		++i)
	{
		result += i;
	}

	return result;
}

inline u32
truncate_f32_u32(f32 value)
{
	return (u32)value;
}

inline u32
round_f32_u32(f32 value)
{
	return (u32)(value + 0.5f);
}

inline i32
round_f32_i32(f32 value)
{
	return (i32)roundf(value);
}

inline f32
distance_square(v2 start, v2 end)
{
	f32 x_diff = end.x - start.x;
	f32 y_diff = end.y - start.y;

	f32 result = x_diff * x_diff + y_diff * y_diff;

	return result;
}

inline u32
get_color_from_v3(v3 color)
{
	u32 r = round_f32_u32(255.0f * color.r);
	u32 g = round_f32_u32(255.0f * color.g);
	u32 b = round_f32_u32(255.0f * color.b);
	u32 result = (0xff << 24) | (r << 16) | (g << 8) | (b << 0);

	return result;
}

internal long long 
factorial(int n)
{
	long long result = 1;
	for (int i = 1;
		i <= n;
		++i)
	{
		result *= i;
	}

	return result;
}

inline f32
clamp(f32 min, f32 value, f32 max)
{
	f32 result = value;
	if (result < min)
	{
		result = min;
	}
	else if (result > max)
	{
		result = max;
	}

	return result;
}

#endif