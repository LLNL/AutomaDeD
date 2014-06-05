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
 * range_set.h
 *
 *  Created on: Feb 22, 2012
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

#ifndef RANGE_SET_H_
#define RANGE_SET_H_

#include <vector>
#include <string>

#include <mpi.h>

using namespace std;

typedef struct {
	unsigned int low;
	unsigned int high;
} Range;


class RangeSet {
private:
	vector<Range> rangeList;

	// Binary search
	int findClosestRange(const size_t &f, const size_t &l,
			const Range &r, size_t &index) const;

	static bool areJoinable(const Range &x, const Range &y);
	static Range joinRanges(const Range &x, const Range &y);
	bool rangeGoesAfterNode(const size_t &i, const Range &r) const;
	bool rangeGoesBeforeNode(const size_t &i, const Range &r) const;

public:
	RangeSet(unsigned int v);
	RangeSet(unsigned int l, unsigned int h);
	RangeSet(const RangeSet &);

	
	RangeSet & operator = (const RangeSet &r);
	// Union of two non-overlapping ranges
	RangeSet & operator += (const RangeSet &r);
	const RangeSet operator + (const RangeSet &r) const;

	string toString() const;
	unsigned int getNumberOfTasks() const;
        unsigned int lowestItem()
	{
            if(rangeList.size() == 0)
		    return -1;

	    return rangeList[0].low;
	};

	bool isPresent(unsigned int);
	bool isPresentInRangeSet(unsigned int, int,int);

	// Returns the number of bytes required to pack it via MPI
	int packed_size(MPI_Comm comm) const;

	// Pack it via MPI
	void pack(void *buf, int bufsize, int *position, MPI_Comm comm) const;

	// Unpack it via MPI
	static RangeSet unpack(void *buf, int bufsize, int *position, MPI_Comm comm);

	friend class Test_RangeSet;
	friend class RangeSetTable;

protected:
	RangeSet() {};
};

#endif /* RANGE_SET_H_ */
