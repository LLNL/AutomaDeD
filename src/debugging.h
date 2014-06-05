//////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2014, Lawrence Livermore National Security, LLC.
// Produced at the Lawrence Livermore National Laboratory.
//
// This file is part of AutomaDeD.
// Written by:
// Ignacio Laguna:lagunaperalt1@llnl.gov, Subrata Mitra: mitra4@purdue.edu.
// All rights reserved.
// LLNL-CODE-647182
//
// For details, see https://github.com/scalability-llnl/AutomaDeD
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License (as published by
// the Free Software Foundation) version 2.1 dated February 1999.
//
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the IMPLIED WARRANTY OF
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the terms and
// conditions of the GNU General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with this program; if not, write to the Free Software Foundation,
// Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
//////////////////////////////////////////////////////////////////////////////
/*
 * debugging.h
 *
 * Debugging and error-handling functions of the library.
 *
 *  Created on: Sep 20, 2010
 *      Author: Ignacio Laguna
 *     Contact: ilaguna@purdue.edu
 */

#ifndef DEBUGGING_H_
#define DEBUGGING_H_

//#if HAVE_CONFIG_H
#include "statetracker-config.h"
//#endif

#include <cstdlib>
#include <cstring>
#include <iostream>

#define ASSERT(condition, message) { \
	if ( !(condition) ) { \
		std::cout << "\t [STracker] Assertion failed: " << std::string(message) << "\n"; \
		std::cout << " on line " << __LINE__  << "\n"; \
		std::cout << " in file " << __FILE__ << "\n";  \
		std::cout.flush(); \
		exit(EXIT_FAILURE); \
    } \
}

/*
 * User-defined malloc function to handle out-of-memory errors.
 */
void * xmalloc (size_t size);

/*
 * Error handling function. Since this is a debugging tool
 * we try to exit as soon as possible when something is wrong.
 */
void handleError(const char *);

/*
 * Logging function. It basically prints messages to stdout.
 */
void log(const char *);

/*--------------------------------------------------------------------*/
/* - Spin function that allows a debugger to attach to the process    */
/*--------------------------------------------------------------------*/
void spinHere();
void printHostInfo();

#endif /* DEBUGGING_H_ */
