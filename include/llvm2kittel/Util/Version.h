// This file is part of llvm2KITTeL
//
// Copyright 2010-2014 Stephan Falke
//
// Licensed under the University of Illinois/NCSA Open Source License.
// See LICENSE for details.

#ifndef VERSION_H
#define VERSION_H

#define VERSION(major, minor) (((major) << 8) | (minor))

#define LLVM_VERSION VERSION(LLVM_MAJOR, LLVM_MINOR)

#endif
