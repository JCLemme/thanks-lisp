#include "builtins.h"

/* Lisp operations
 * Math related
 */

Cell* fn_add(Cell* args) // (+ &rest n) Adds all numbers n
{
    double acc = 0;
    while(!IS_NIL(args))
    {
        Cell* arg = args->car;
        if(!IS_TYPE(arg->tag, TAG_TYPE_NUMBER))
        {
            return memory_alloc_exception(TAG_SPEC_EX_TYPE, memory_alloc_string("add: not a number", -1));
        }
        acc += arg->num;
        args = args->cdr;
    }

    return memory_alloc_number(acc);
}

Cell* fn_sub(Cell* args) // (- n &rest s) Subtracts all numbers s from number n, or negates n if s is nil
{
    double acc = 0;
    bool first = true;
    int argc = memory_length(args);

    if(argc == 1)
    {
        Cell* arg = args->car;
        return memory_alloc_number(arg->num * -1);
    }
    else
    {
        while(!IS_NIL(args))
        {
            Cell* arg = args->car;
            if(!IS_TYPE(arg->tag, TAG_TYPE_NUMBER))
            {
                return memory_alloc_exception(TAG_SPEC_EX_TYPE, memory_alloc_string("sub: not a number", -1));
            }
            if(first)
            {
                acc = arg->num;
                first = false;
            }
            else
            {
                acc -= arg->num;
            }

            args = args->cdr;
        }

        return memory_alloc_number(acc);
    }
}

Cell* fn_mul(Cell* args) // (* n &rest m) Multiplies number n by all numbers m, or returns n if m is nil
{
    double acc = 1;
    while(!IS_NIL(args))
    {
        Cell* arg = args->car;
        if(!IS_TYPE(arg->tag, TAG_TYPE_NUMBER))
        {
            return memory_alloc_exception(TAG_SPEC_EX_TYPE, memory_alloc_string("mul: not a number", -1));
        }
        acc *= arg->num;
        args = args->cdr;
    }

    return memory_alloc_number(acc);
}

Cell* fn_div(Cell* args) // (/ n &rest d) Divides number n by all numbers d
{
    double acc = 0;
    bool first = true;
    while(!IS_NIL(args))
    {
        Cell* arg = args->car;
        if(!IS_TYPE(arg->tag, TAG_TYPE_NUMBER))
        {
            return memory_alloc_exception(TAG_SPEC_EX_TYPE, memory_alloc_string("div: not a number", -1));
        }
        if(first)
        {
            acc = arg->num;
            first = false;
        }
        else
        {
            acc /= arg->num;
        }

        args = args->cdr;
    }

    return memory_alloc_number(acc);
}

Cell* fn_mod(Cell* args) // (mod n d) Returns the remainder left after dividing number n by number d
{
    Cell* num = memory_nth(args, 0)->car;
    Cell* den = memory_nth(args, 1)->car;

    return memory_alloc_number((int)num->num % (int)den->num);
}

Cell* fn_sin(Cell* args) // (sin a) Finds the sine of an angle a, in radians
{
    Cell* num = memory_nth(args, 0)->car;

    return memory_alloc_number(sin(num->num));
}

Cell* fn_cos(Cell* args) // (cos a) Finds the cosine of an angle a, in radians
{
    Cell* num = memory_nth(args, 0)->car;

    return memory_alloc_number(cos(num->num));
}

Cell* fn_tan(Cell* args) // (tan a) Finds the tangent of an angle a, in radians
{
    Cell* num = memory_nth(args, 0)->car;

    return memory_alloc_number(tan(num->num));
}

Cell* fn_sqrt(Cell* args) // (sqrt n) Returns the square root of a number n
{
    Cell* num = memory_nth(args, 0)->car;

    return memory_alloc_number(sqrt(num->num));
}

Cell* fn_pow(Cell* args) // (pow n p) Raises a number n to a power p
{
    Cell* num = memory_nth(args, 0)->car;
    Cell* den = memory_nth(args, 1)->car;

    return memory_alloc_number(pow(num->num, den->num));
}

Cell* fn_truncate(Cell* args) // (truncate n) Returns the integer part of a number n, rounding towards zero
{
    Cell* num = memory_nth(args, 0)->car;

    return memory_alloc_number(trunc(num->num));
}
Cell* fn_gt(Cell* args) // (> a b) Returns true if number a is greater than number b
{
    Cell* a = memory_nth(args, 0)->car;
    Cell* b = memory_nth(args, 1)->car;
    return (a->num > b->num) ? T : NIL;
}

Cell* fn_gteq(Cell* args) // (>= a b) Returns true if number a is greater than or equal to number b
{
    Cell* a = memory_nth(args, 0)->car;
    Cell* b = memory_nth(args, 1)->car;
    return (a->num >= b->num) ? T : NIL;
}

Cell* fn_lt(Cell* args) // (< a b) Returns true if number a is less than number b
{
    Cell* a = memory_nth(args, 0)->car;
    Cell* b = memory_nth(args, 1)->car;
    return (a->num < b->num) ? T : NIL;
}

Cell* fn_lteq(Cell* args) // (<= a b) Returns true if number a is less than or equal to number b
{
    Cell* a = memory_nth(args, 0)->car;
    Cell* b = memory_nth(args, 1)->car;
    return (a->num <= b->num) ? T : NIL;
}

Cell* fn_numeq(Cell* args) // (== a b) Returns true if numbers a and b are equal
{
    // These are here for compat with CL.
    Cell* a = memory_nth(args, 0)->car;
    Cell* b = memory_nth(args, 1)->car;
    return (a->num == b->num) ? T : NIL;
}

Cell* fn_numneq(Cell* args) // (!= a b) Returns true if numbers a and b are inequal
{
    // I mean I guess they could point to the same builtin but 
    Cell* a = memory_nth(args, 0)->car;
    Cell* b = memory_nth(args, 1)->car;
    return (a->num != b->num) ? T : NIL;
}

