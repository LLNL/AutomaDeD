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
 * debugging.cxx
 *
 *  Created on: Sep 20, 2010
 *      Author: Ignacio Laguna
 *     Contact: ilaguna@purdue.edu
 */

//#if HAVE_CONFIG_H
#include "statetracker-config.h"
//#endif

#include <stdlib.h>
#include <stdio.h>
#include <iostream>
#include <unistd.h>
#include <string>
#include <mpi.h>
#include "debugging.h"

using namespace std;

void *
xmalloc (size_t size)
{
	register void *value = malloc(size);
    if (value == 0)
    	handleError("in xmalloc(): virtual memory exhausted.\n");
    return value;
}

void
handleError(const char *message)
{
	cerr << "[AUT]: Error: " << string(message) << endl;
	cerr.flush();
	exit(EXIT_FAILURE);
}

void
log(const char *message)
{
	cout << "[AUT] " << string(message);
	cout.flush();
	return;
}

void spinHere() {
    volatile int i = 0;
    char hostname[256];
    gethostname(hostname, sizeof(hostname));
    printf("PID %d on %s ready for attach\n", getpid(), hostname);
    fflush(stdout);
    while (0 == i)
        sleep(5);
}

void printHostInfo() {
    char hostname[256];
    gethostname(hostname, sizeof(hostname));
    printf("PID %d running on %s\n", getpid(), hostname);
    fflush(stdout);
}

