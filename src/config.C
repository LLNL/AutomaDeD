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
 * config.cxx
 *
 *  Created on: Apr 9, 2011
 *      Author: Ignacio Laguna
 *     Contact: ilaguna@purdue.edu
 *        Modified on: Nov 2, 2013
 *         *    Author: Subrata Mitra
 *          *    Contact: mitra4@purdue.edu
 *
 */

//#if HAVE_CONFIG_H
#include "statetracker-config.h"
//#endif

#include "config.h"
#include "debugging.h"

#include <stdlib.h>
#include <string>

using namespace std;

bool AUTConfig::getBoolParameter(const char *variable, bool &value)
{
	bool ret = false;
	char * parameter = getenv(variable);
	if (parameter != NULL) {
		if (string(parameter).compare("TRUE") == 0) {
			value = true;
			ret = true;
		} else if (string(parameter).compare("FALSE") == 0) {
			value = false;
			ret = true;
		} else {
			string msg = string("unknown parameter: ") + string(variable);
			handleError(msg.c_str());
		}
	}

	return ret;
}

bool AUTConfig::getIntParameter(const char * variable, int &value)
{
	bool ret = false;
	char * parameter = getenv(variable);
	if (parameter != NULL) {
		value = atoi(parameter);
		ret = true;
	}

	return ret;
}

bool AUTConfig::getDoubleParameter(const char * variable, double &value)
{
	bool ret = false;
	char * parameter = getenv(variable);
	if (parameter != NULL) {
		value = atof(parameter);
		ret = true;
	}

	return ret;
}
