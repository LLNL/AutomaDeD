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
 * test_RangeSet.C
 *
 *  Created on: Feb 24, 2012
 *      Author: Ignacio Laguna
 *     Contact: ilaguna@purdue.edu
 */

#include "test_RangeSet.h"
#include "range_set.h"
#include "assert_functions.h"

#include <iostream>

using namespace std;

void Test_RangeSet::canPerformBinarySearchInRangeSet()
{
	RangeSet rs;

	Range range;
	range.low = range.high = 5;
	rs.rangeList.push_back(range);

	range.low = 26;
	range.high = 50;
	rs.rangeList.push_back(range);

	range.low = 70;
	range.high = 70;
	rs.rangeList.push_back(range);

	range.low = 78;
	range.high = 82;
	rs.rangeList.push_back(range);

	// Print
	/*for (size_t i=0; i < rs.rangeList.size(); ++i) {
		cout << "[" << i << "]: ";
		if (rs.rangeList[i].low == rs.rangeList[i].high)
			cout << rs.rangeList[i].low;
		else
			cout << rs.rangeList[i].low << "-" << rs.rangeList[i].high;
		cout << endl;
	}*/

	// Tests: [5, 25-50, 70, 78-82]
	size_t index;
	int res;

	range.low = 1;
	range.high = 2;
	res = rs.findClosestRange(0, rs.rangeList.size()-1, range, index);
	bool cond1 = (index==0) && (res==-1);

	range.low = 7;
	range.high = 7;
	res = rs.findClosestRange(0, rs.rangeList.size()-1, range, index);
	bool cond2 = (index==0) && (res==1);

	range.low = 62;
	range.high = 69;
	res = rs.findClosestRange(0, rs.rangeList.size()-1, range, index);
	bool cond3 = (index==1) && (res==1);

	range.low = 100;
	range.high = 200;
	res = rs.findClosestRange(0, rs.rangeList.size()-1, range, index);
	bool cond4 = (index==3) && (res==1);

	range.low = 70; // error because it overlaps
	range.high = 75;
	res = rs.findClosestRange(0, rs.rangeList.size()-1, range, index);
	bool cond5 = (res==0 || res==-2);

	range.low = 10;
	range.high = 26; // error because it overlaps
	res = rs.findClosestRange(0, rs.rangeList.size()-1, range, index);
	bool cond6 = (res==0 || res==-2);

	range.low = 26;
	range.high = 26; // error because it overlaps
	res = rs.findClosestRange(0, rs.rangeList.size()-1, range, index);
	bool cond7 = (res==0 || res==-2);

	//cout << "[" << range.low << "-" << range.high << "]" << "\t";
	//cout << "Index: " << index << " dir: " << res << endl;
	//cout << cond1 << " " << cond2 << " " << cond3 <<
	//		" " << cond4 << " " << cond5 << endl;

	ASSERT_TRUE((cond1 && cond2 && cond3 && cond4 && cond5 && cond6 && cond7),
				("Can perform binary search in RangeSet"));
}

void Test_RangeSet::canAddRangeSets()
{
	RangeSet rs(5);
	rs = rs + RangeSet(10,11);
	//cout << rs.toString() << endl;
	bool cond1 = rs.toString().compare("[5,10-11]") == 0;

	rs += RangeSet(1,2);
	//cout << rs.toString() << endl;
	bool cond2 = rs.toString().compare("[1-2,5,10-11]") == 0;

	rs += RangeSet(100,100);
	//cout << rs.toString() << endl;
	bool cond3 = rs.toString().compare("[1-2,5,10-11,100]") == 0;

	rs += RangeSet(12,30);
	//cout << rs.toString() << endl;
	bool cond4 = rs.toString().compare("[1-2,5,10-30,100]") == 0;

	rs += RangeSet(51,63);
	//cout << rs.toString() << endl;
	bool cond5 = rs.toString().compare("[1-2,5,10-30,51-63,100]") == 0;

	rs += RangeSet(64,64);
	//cout << rs.toString() << endl;
	bool cond6 = rs.toString().compare("[1-2,5,10-30,51-64,100]") == 0;

	rs += RangeSet(9,9);
	//cout << rs.toString() << endl;
	bool cond7 = rs.toString().compare("[1-2,5,9-30,51-64,100]") == 0;

	rs += RangeSet(101,101);
	//cout << rs.toString() << endl;
	bool cond8 = rs.toString().compare("[1-2,5,9-30,51-64,100-101]") == 0;

	rs += RangeSet(0,0);
	//cout << rs.toString() << endl;
	bool cond9 = rs.toString().compare("[0-2,5,9-30,51-64,100-101]") == 0;

	ASSERT_TRUE((cond1 && cond2 && cond3 && cond4 && cond5 &&
			cond6 && cond7 && cond8 && cond9),
					("Can add RangeSets"));
}
