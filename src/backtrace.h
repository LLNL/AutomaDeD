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
 * backtrace.h
 *
 *  Created on: Dec 12, 2011
 *      Author: Ignacio Laguna
 *     Contact: ilaguna@purdue.edu
 *         Modified on: Nov 2, 2013
 *          *    Author: Subrata Mitra
 *           *    Contact: mitra4@purdue.edu
 */

#ifndef BACKTRACE_H_
#define BACKTRACE_H_

//#ifdef HAVE_CONFIG_H
#include "statetracker-config.h"
//#endif

#ifdef STATE_TRACKER_WITH_CALLPATH
#include "CallpathRuntime.h"
#endif

#include <string>

using namespace std;

typedef struct {
	string fileNameAndLine;
	string functionName;
	bool fromTool;
} FileAndFunction;

class Backtrace {
private:
	static string add2lineProgram;
	static string objdumpProgram;
#ifdef STATE_TRACKER_WITH_CALLPATH
        static CallpathRuntime runtime;
	static bool usecallpath;
	static string getBackTraceFromCallPath();
#endif
	static string getGNUBacktrace();
	//static string getStackwalkerBacktrace();
public:
	static string getBacktrace();

	/**
	 * Address-to-line tools
	 */
	static bool canConvertAddrToLine();
	static string convertAddrToLine(const string &addr, const string &program);

	/**
	 * Address search tools
	 */
	static bool canFindFileAndFunctionFromObject();
	static FileAndFunction findFileAndFunctionFromObject(const string &functOffset);
};

#endif /* BACKTRACE_H_ */
