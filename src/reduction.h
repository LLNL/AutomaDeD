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
 * reduction.h
 *
 * Template classes to perform different tree-based reduction algorithms.
 *
 *  Created on: Jan 6, 2011
 *      Author: Ignacio Laguna
 *     Contact: ilaguna@purdue.edu
 */

#ifndef REDUCTION_H_
#define REDUCTION_H_

//#if HAVE_CONFIG_H
#include "statetracker-config.h"
//#endif

#include <math.h>
#include <iostream>
using namespace std;

/**
 * Abstraction to calculate 'senders' and 'receivers'
 * in a tree-based reduction.
 *
 * The class T needs to implement two member functions:
 * send(int destination)
 * receive(int source)
 *
 * The reduction operations are performed inside these functions.
 */
template <class T>
class Reducer {
protected:
	Reducer(){}
public:
	virtual ~Reducer(){}

	/**
	 * Virtual reduction operation (implemented in derived classes)
	 */
	virtual void reduce(int root, int numProcesses, int processRank, T *reducedObject) = 0;
};

/*
 * ----------------------------------
 * Binomial Tree Reduction from MPICH
 * ----------------------------------
 *
 * This algorithm is adapted from MPICH. It performs a binomial-tree reduction
 * assuming that the operations are commutative. The root process can be
 * specified in the reduce() function.
 */
template <class T>
class BinomialReducer : public Reducer<T> {
public:
	void reduce(int root, int numProcesses, int processRank, T *reducedObject)
	{
		int mask = 0x1, relrank, source, destination;
		relrank = (processRank - root + numProcesses) % numProcesses;
		while (mask < numProcesses) {
			// Receive
			if ((mask & relrank) == 0) {
				source = (relrank | mask);
				if (source < numProcesses) {
					source = (source + root) % numProcesses;
					//cout << "Proc " << processRank << " recv from " << source << endl;
					reducedObject->receive(source);
				}
			} else {
				// I've received all that I'm going to.  Send my result to my parent
				destination = ((relrank & (~ mask)) + root) % numProcesses;
				//cout << "Proc " << processRank << " send to " << destination << endl;
				reducedObject->send(destination);
				break;
			}
			mask <<= 1;
		}
	}
};

/*
 * ---------------------------
 * My own reduction algorithm
 * ---------------------------
 *
 * It is used for debugging purposes only. The root parameter is ignored
 * and the root is assumed to be 0.
 */
template <class T>
class IntraReducer : public Reducer<T> {
public:
	void reduce(int root, int numProcesses, int processRank, T *reducedObject)
	{
		// Number of iterations. This is the length of the tree.
		int num_iterations = static_cast<int>(ceil(log10(static_cast<double>(numProcesses)) / log10(2.0)));

		for (int i=1; i <= num_iterations; i++) {
			int divisor = static_cast<int>(pow(2.0, static_cast<double>(i))); // the divisor in modulo operation to determine roles
			int role = processRank % divisor; // if role == 0, the process is a receiver, otherwise it's a sender

			if (role == 0) {
				// Determine sender ID (whom to wait for?)
				int sender = static_cast<int>(static_cast<double>(divisor) / 2.0 + static_cast<double>(processRank));
				if (sender > (numProcesses - 1))
					continue; // Do not wait for nonexistent process
				else {
					//cout << "Process: " << myid << " receiving from " << sender << endl;
					reducedObject->receive(sender);
				}
			} else {
				// Determine receiver ID.
				int receiver = processRank - role;
				//cout << "Process: " << myid << " sending to " << receiver << endl;
				reducedObject->send(receiver);
				break; // after we send, we're done!
			}
		}
	}
};


#endif /* REDUCTION_H_ */
