// EPOS Math Utility Declarations

#ifndef __math_h
#define __math_h

#include <system/config.h>

__BEGIN_UTIL

static const float E = 2.71828183;

template <typename T>
inline T logf(T num, float base = E, float epsilon = 1e-12)
{
    if(num == 0) return 1;

    if(num < 1  && base < 1) return 0;

    T integer = 0;
    while(num < 1) {
        integer--;
        num *= base;
    }

    while(num >= base) {
        integer++;
        num /= base;
    }

    T partial = 0.5;
    num *= num;
    T decimal = 0.0;
    while(partial > epsilon) {
        if(num >= base) {
            decimal += partial;
            num /= base;
        }
        partial *= 0.5;
        num *= num;
    }

    return integer + decimal;
}

template <typename T>
inline T sqrt(const T & x)
{
    T xhi = x;
    T xlo = 0;
    T guess = x/2;

    while (guess * guess != x) {
        if (guess * guess > x)
            xhi = guess;
        else
            xlo = guess;

        float new_guess = (xhi + xlo) / 2;
        if (new_guess == guess)
            break; // not getting closer
        guess = new_guess;
    }

    return guess;
}

inline float fast_log2(float val)
{
    int * const exp_ptr = reinterpret_cast <int *> (&val);
    int x = *exp_ptr;
    const int log_2 = ((x >> 23) & 255) - 128;
    x &= ~(255 << 23);
    x += 127 << 23;
    (*exp_ptr) = x;

    val = ((-1.0f/3) * val + 2) * val - 2.0f/3;

    return (val + log_2);
}

inline float fast_log(float val)
{
    static const float ln_2 = 0.69314718;
    return (fast_log2(val) * ln_2);
}

template <typename T>
const T & min(const T & x, const T & y)
{
    return (x <= y) ? x : y;
}

template <typename T>
const T & max(const T & x, const T & y)
{
    return (x > y) ? x : y;
}

template <typename T>
T abs(const T & x)
{
    if(x > 0) return x;
    return -x;
}

template <typename T>
T largest(const T array[], int size)
{
    T result = array[0];
    for(int i = 1; i <  size; i++)
        if(array[i] > result)
          result = array[i];
    return result;
}

template <typename T>
T smallest(const T array[], int size)
{
    T result = array[0];
    for(int i = 1; i <  size; i++)
        if(array[i] < result)
          result = array[i];
    return result;
}

template <typename T>
T mean(const T array[], int size)
{
    T sum = 0;
    for(int i = 0; i < size; i++)
        sum += array[i];
    return sum / size;
}

template <typename T>
T variance(const T array[], int size, const T & mean)
{
    T var = 0;
    for(int i = 0; i < size; i++) {
        T tmp = mean - array[i];
        var = var + (tmp * tmp);
    }
    return var / (size - 1);
}

__END_UTIL

#endif
