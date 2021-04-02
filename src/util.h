#ifndef UTIL_H
#define UTIL_H

#define ncmp(x, y)                                                             \
	_Generic((x), long                                                     \
		 : _Generic((y), long                                          \
			    : cmp_long, double                                 \
			    : cmp_long_double),                                \
		   double                                                      \
		 : _Generic((y), long                                          \
			    : cmp_double_long, double                          \
			    : cmp_double))(x, y)

int cmp_long(long x, long y);
int cmp_double(double x, double y);
int cmp_long_double(long x, double y);
int cmp_double_long(double x, long y);

void swap(long *a, long *b);
long gcd(long a, long b);

#endif /* ifndef UTIL_H */
