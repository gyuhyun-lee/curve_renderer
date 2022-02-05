#ifndef CURVE_H
#define CURVE_H

struct ControlPoint
{
	v2 p;
};

enum CurveMethod
{
	curve_method_nli,
	curve_method_bernstein,
	curve_method_midpoint,
};

#endif