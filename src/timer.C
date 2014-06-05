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
 * timer.cxx
 *
 *  Created on: Feb 28, 2011
 *      Author: Ignacio Laguna
 *     Contact: ilaguna@purdue.edu
 */

//#if HAVE_CONFIG_H
#include "statetracker-config.h"
//#endif

#include "timer.h"
#include <time.h>
#include <sys/time.h>
#include <map>
#include <string>
#include <vector>
#include <iostream>
#include <algorithm>

using namespace std;

void MultiTimer::start(int rank) {
	if (rank != 0) return;
	gettimeofday(&gtime, NULL);
}

double MultiTimer::stop(int rank, const char * varName) {
	if (rank != 0) return 0;

	struct timeval newtime;
	gettimeofday(&newtime, NULL);

	// Get difference in time
	double time1 = static_cast<double>(gtime.tv_sec) +
			static_cast<double>(gtime.tv_usec)/(1.0e6);
	double time2 = static_cast<double>(newtime.tv_sec) +
			static_cast<double>(newtime.tv_usec)/(1.0e6);
	double difference = time2 - time1;

	// Add value to times-table
	map<string, vector<double> >::iterator it =
			timesTable.find(string(varName));
	if (it == timesTable.end()) {
		vector<double> v(1);
		v[0] = difference;
		timesTable.insert(pair<string, vector<double> >(string(varName),v));
	} else {
		it->second.push_back(difference);
	}

	return difference;
}

void MultiTimer::printTimes() const {
	if (timesTable.size() == 0)
		return;  // Nothing to print

	cout << "------------Times-------------" << endl;
	cout << "[Variable] [Average] [Maximum]" << endl;
	cout << "------------------------------" << endl;

	map<string, vector<double> >::const_iterator it;
	for (it = timesTable.begin(); it != timesTable.end(); it++) {
		double average=0;
		for (size_t i = 0; i < it->second.size(); ++i)
			average += it->second[i];
		average = average / static_cast<double>(it->second.size());

		double max = *max_element(it->second.begin(), it->second.end());

		cout << it->first << " " << scientific << average << " " << max << endl;
	}
	cout << "------------------------------" << endl;
}



