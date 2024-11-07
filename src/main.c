#include <stdio.h>
#include <stdint.h>
#include "fdir.h"

void function_l(int k)
{
    int l = 0x11111111;
    l ^= k;
    l = (l << 4) | (l >> 28);

    printf("l = %d\n", l);

    // Enable div by 0 exception
    *((volatile uint32_t *)0xE000ED14) |= (1 << 4);

    // Trigger the error
    volatile uint32_t zero = 0;
    volatile uint32_t result = 1 / zero;

    (void)result;
}

void function_k(int j)
{
    int k = 0x22222222;
    k += j;
    k = ~k;

    printf("k = %d\n", k);

    function_l(k);
}

void function_j(int i)
{
    int j = 0x33333333;
    j ^= i;
    j = (j >> 2) + (j << 5);

    printf("j = %d\n", j);

    function_k(j);
}

void function_i(int h)
{
    int i = 0x44444444;
    i += h;
    i = (i << 1) | (i >> 31);

    printf("i = %d\n", i);

    function_j(i);
}

void function_h(int g)
{
    int h = 0x55555555;
    h ^= g;
    h = ~h;

    printf("h = %d\n", h);

    function_i(h);
}

void function_g(int f)
{
    int g = 0x66666666;
    g += f;
    g = (g >> 3) + (g << 3);

    printf("g = %d\n", g);

    function_h(g);
}

void function_f(int e)
{
    int f = 0x77777777;
    f ^= e;
    f = (f << 2) | (f >> 30);

    printf("f = %d\n", f);

    function_g(f);
}

void function_e(int d)
{
    int e = 0x88888888;
    e += d;
    e = ~e;

    printf("e = %d\n", e);

    function_f(e);
}

void function_d(int c)
{
    int d = 0x99999999;
    d ^= c;
    d = (d >> 1) + (d << 4);

    printf("d = %d\n", d);

    function_e(d);
}

void function_c(int b)
{
    int c = 0xaaaaaaaa;
    c += b;
    c = (c << 3) | (c >> 29);

    printf("c = %d\n", c);

    function_d(c);
}

void function_b(int a)
{
    int b = 0xbbbbbbbb;
    b ^= a;
    b = ~b;

    printf("b = %d\n", b);

    function_c(b);
}

void function_a(int y)
{
    int a = 0xcccccccc;
    a += y;
    a = (a >> 2) + (a << 6);

    printf("a = %d\n", a);

    function_b(a);
}


int main(void) {
    InitFDIR();

    // Causes HardFault
    function_a(10);
    //function_l(10);

    while (1) {
        // Infinite loop
    }
    return 0;
}
