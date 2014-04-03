// This file is part of llvm2KITTeL
//
// Copyright 2010-2014 Stephan Falke
//
// Licensed under the University of Illinois/NCSA Open Source License.
// See LICENSE for details.

#include "Polynomial.h"

void mpz_kittel_set_simax(mpz_t rop, unsigned int bitwidth)
{
    mpz_set(rop, Polynomial::_one);
    mpz_mul_2exp(rop, rop, bitwidth - 1);
    mpz_sub(rop, rop, Polynomial::_one);
}

void mpz_kittel_set_simin_as_ui(mpz_t rop, unsigned int bitwidth)
{
    mpz_set(rop, Polynomial::_one);
    mpz_mul_2exp(rop, rop, bitwidth - 1);
}

void mpz_kittel_set_simin(mpz_t rop, unsigned int bitwidth)
{
    mpz_kittel_set_simin_as_ui(rop, bitwidth);
    mpz_neg(rop, rop);
}

void mpz_kittel_set_uimax(mpz_t rop, unsigned int bitwidth)
{
    mpz_set(rop, Polynomial::_one);
    mpz_mul_2exp(rop, rop, bitwidth);
    mpz_sub(rop, rop, Polynomial::_one);
}

void mpz_kittel_set_power_of_two(mpz_t rop, unsigned int power)
{
    mpz_set(rop, Polynomial::_one);
    mpz_mul_2exp(rop, rop, power);
}
