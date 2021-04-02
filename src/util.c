#include "util.h"
#include <limits.h>
#include <math.h>
#define DBL_LONG_MAX_P1 ((LONG_MAX / 2 + 1) * 2.0)

int cmp_long(long x, long y) { return (x > y) - (x < y); }

int cmp_double(double x, double y) { return (x > y) - (x < y); }

int cmp_long_double(long x, double y)
{
	// These two compares are expected to be exact.
	if (y >= DBL_LONG_MAX_P1) {
		return -1;
	} else if (y < (double)LONG_MIN) {
		return 1;
	}

	// (long)y is now in range of `long` (Aside from NaNs).
	long y_long = (long)y; // Lose the fraction
	if (y_long > x) {
		return -1;
	} else if (y_long < x) {
		return 1;
	}

	// Still equal, so look at the fraction.
	double whole;
	double fraction = modf(y, &whole);
	if (fraction > 0.0) {
		return -1;
	} else if (fraction < 0.0) {
		return 1;
	} else {
		return 0;
	}
}

inline int cmp_double_long(double x, long y) { return -cmp_long_double(y, x); }

void swap(long *a, long *b)
{
	long tmp = *a;
	*a = *b;
	*b = tmp;
}

long gcd(long a, long b)
{
	while (b) {
		a -= b * (a / b);
		swap(&a, &b);
	}
	return a;
}
