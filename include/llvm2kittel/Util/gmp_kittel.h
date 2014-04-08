// This file is part of llvm2KITTeL
//
// Copyright 2010-2014 Stephan Falke
//
// Licensed under the University of Illinois/NCSA Open Source License.
// See LICENSE for details.

#ifndef GMP_KITTEL_H
#define GMP_KITTEL_H

#include <gmpxx.h>

void mpz_kittel_set_simax(mpz_t rop, unsigned int bitwidth);

void mpz_kittel_set_simin_as_ui(mpz_t rop, unsigned int bitwidth);

void mpz_kittel_set_simin(mpz_t rop, unsigned int bitwidth);

void mpz_kittel_set_uimax(mpz_t rop, unsigned int bitwidth);

void mpz_kittel_set_power_of_two(mpz_t rop, unsigned int power);

#endif // GMP_KITTEL_H
