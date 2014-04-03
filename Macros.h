// This file is part of llvm2KITTeL
//
// Copyright 2010-2014 Stephan Falke
//
// Licensed under the University of Illinois/NCSA Open Source License.
// See LICENSE for details.

#ifndef MACROS_H
#define MACROS_H

#if LLVM_MAJOR < 3
  #define LLVM2_CONST const
#else
  #define LLVM2_CONST
#endif

#endif
