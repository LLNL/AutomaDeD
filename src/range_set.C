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
 * range_set.C
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

#include "range_set.h"
#include "debugging.h"

#include <vector>
#include <string>
#include <iterator>
#include <iostream>

#include <stdio.h>

using namespace std;

RangeSet::RangeSet(unsigned int v)
{
	Range r;
	r.low = r.high = v;
	rangeList.push_back(r);
}

RangeSet::RangeSet(unsigned int l, unsigned int h)
{
#if STATE_TRACKER_DEBUG
	if (l>h)
		handleError("in RangeSet constructor: l cannot be > h");
#endif

	Range r;
	r.low = l;
	r.high = h;
	rangeList.push_back(r);
}
RangeSet::RangeSet(const RangeSet & rs)
{
	rangeList = rs.rangeList;
}

RangeSet & RangeSet::operator=(const RangeSet & rs)
{
	 rangeList = rs.rangeList;

	 return (*this);
}                    

RangeSet & RangeSet::operator +=(const RangeSet & r)
{
	size_t index;
	int res;
	for (size_t i=0; i < r.rangeList.size(); ++i) {
		Range range = r.rangeList[i];
		vector<Range>::iterator it;
		res = findClosestRange(0, rangeList.size()-1, range, index);
		if (res == 1) { // insert after index
			it = rangeList.insert(rangeList.begin() + index + 1, range);
		} else if (res == -1) { // insert before index
			it = rangeList.insert(rangeList.begin() + index, range);
		} else {
			cout << "Merging: " << this->toString() << " "
					<< r.toString() << endl;
			handleError("in RangeSet::operator +=: ranges may be overlapping");
		}

		// Check if we can join neighbors
		size_t j = distance(rangeList.begin(), it); //index of inserted element

		if (j > 0) {
			Range before = rangeList[j-1];
			Range current = rangeList[j];
			if (areJoinable(before, current)) {
				Range newRange = joinRanges(before, current);
				rangeList[j] = newRange;
				rangeList.erase(rangeList.begin() + j - 1);
				j--;
			}
		}

		if (j < rangeList.size()-1) {
			Range current = rangeList[j];
			Range after = rangeList[j+1];
			if (areJoinable(current, after)) {
				Range newRange = joinRanges(current, after);
				rangeList[j] = newRange;
				rangeList.erase(rangeList.begin() + j + 1);
			}
		}
	}

	return *this;
}

const RangeSet RangeSet::operator +(const RangeSet & r) const
{
    RangeSet res = *this;
    res += r;
    return res;
}

/*
 * Preserves order, i.e. (x,y) != (y,x)
 */
bool RangeSet::areJoinable(const Range &x, const Range &y)
{
	return ((x.high + 1) == y.low);
}

Range RangeSet::joinRanges(const Range &x, const Range &y)
{
	Range r;
	r.low = x.low;
	r.high = y.high;
	return r;
}
bool RangeSet::isPresent(unsigned int val)
{
   return isPresentInRangeSet(val,0,(rangeList.size() - 1));
}
bool RangeSet::isPresentInRangeSet(unsigned int val,int first, int last)
{    
    if(rangeList.size() == 0)
    {
	    return false;
    }
    if((val < rangeList[first].low) || ( rangeList[last]).high < val)
    {
	    return false;
    }
    else if((rangeList[first].low <= val) && (val <= rangeList[first].high))
    {
	    return true;
    }
    else if((rangeList[last].low <= val ) && (val <= rangeList[last].high))
    {
	    return true;
    }
    else if(first == last)
    {
	    return false;
    }
    
    size_t mid = first + ((last-first + 1)/2);
    if((rangeList[mid].low <= val) && (val <= rangeList[mid].high))
    {
	    return true;
    }
    else if(val < rangeList[mid].low)
    {
       if(mid == 0)
       {
	       return false;
       }
       return isPresentInRangeSet(val,first,mid-1 );
    }
    else if(rangeList[mid].high < val)
    {
       if(mid == (rangeList.size() - 1))
       {
	       return false;
       }
       return isPresentInRangeSet(val,mid+1, last);
    }
    return false;
}
/*
 * Binary search:
 * --------------
 * Find the index (of the range) that is before the passed range.
 * Sign of returned integer indicates direction (before: -1 of after: 1).
 * Returns 0 or -2 when ranges overlap.
 */
int RangeSet::findClosestRange(const size_t &first, const size_t &last,
		const Range &r, size_t &index) const
{
	// The same element in the list (leaves of the tree-based search)
	if (first == (last-1)) {
		if (rangeGoesAfterNode(first, r) && rangeGoesBeforeNode(last, r)) {
			index = first;
			return 1;
		} else if (rangeGoesBeforeNode(first, r)) {
			index = first;
			return -1;
		} else if (rangeGoesAfterNode(last, r)) {
			index = last;
			return 1;
		} else {
			return 0;
		}
	}

	size_t i = first + ((last-first + 1)/2);
	if (rangeGoesAfterNode(i, r)) {
		if (i < rangeList.size()-1) {
			return findClosestRange(i, last, r, index);
		} else {
			index = i;
			return 1;
		}
	} else if (rangeGoesBeforeNode(i, r)) {
		if (i > 0) {
			return findClosestRange(first, i, r, index);
		} else {
			index = i;
			return -1;
		}
	} else {
		return -2;
	}
}

bool RangeSet::rangeGoesAfterNode(const size_t &i, const Range &r) const
{
	return (r.low > rangeList[i].high);
}

bool RangeSet::rangeGoesBeforeNode(const size_t &i, const Range &r) const
{
	return (r.high < rangeList[i].low);
}

string RangeSet::toString() const
{
	string ret("[");
	for (size_t i=0; i < rangeList.size(); ++i) {
		if (rangeList[i].low == rangeList[i].high) {
			char buff[256];
			sprintf(buff, "%d", rangeList[i].low);
			ret += string(buff);
		} else {
			char buff[256];
			sprintf(buff, "%d", rangeList[i].low);
			ret += string(buff) + "-";
			sprintf(buff, "%d", rangeList[i].high);
			ret += string(buff);
		}

		if (i == rangeList.size()-1)
			ret += "]";
		else
			ret += ",";
	}

	return ret;
}

int RangeSet::packed_size(MPI_Comm comm) const
{
	int size=0, tmp=0;

	// Integer for number of ranges in the vector
	PMPI_Pack_size(1, MPI_UNSIGNED, comm, &tmp);
	size += tmp;

	size_t numElems = rangeList.size();
	PMPI_Pack_size(1, MPI_UNSIGNED, comm, &tmp);
	size += numElems*(tmp*2);

	return size;
}

void RangeSet::pack(void *buf, int bufsize, int *position, MPI_Comm comm) const
{
	// Pack number number of ranges in the vector
	unsigned int n = static_cast<unsigned int>(rangeList.size());
	PMPI_Pack((void *)&n, 1, MPI_UNSIGNED, buf, bufsize, position, comm);

	for (size_t i=0; i < rangeList.size(); ++i) {
		unsigned int x = rangeList[i].low;
		unsigned int y = rangeList[i].high;
		PMPI_Pack((void *)&x, 1, MPI_UNSIGNED, buf, bufsize, position, comm);
		PMPI_Pack((void *)&y, 1, MPI_UNSIGNED, buf, bufsize, position, comm);
	}
}

RangeSet RangeSet::unpack(void *buf, int bufsize, int *position, MPI_Comm comm)
{
	unsigned int n = 0;
	PMPI_Unpack(buf, bufsize, position, &n, 1, MPI_UNSIGNED, comm);

#if STATE_TRACKER_DEBUG
	if (n == 0)
		handleError("in RangeSet::unpack: number of states is 0");
#endif

	RangeSet rs;
	for (size_t i=0; i < n; ++i) {
		unsigned int x,y;
		PMPI_Unpack(buf, bufsize, position, &x, 1, MPI_UNSIGNED, comm);
		PMPI_Unpack(buf, bufsize, position, &y, 1, MPI_UNSIGNED, comm);
		Range r;
		r.low = x;
		r.high = y;
		rs.rangeList.push_back(r);
	}

	return rs;
}

unsigned int RangeSet::getNumberOfTasks() const
{
	unsigned int ret = 0;
	for (size_t i=0; i < rangeList.size(); ++i)
		ret += rangeList[i].high - rangeList[i].low + 1;

	return ret;
}
