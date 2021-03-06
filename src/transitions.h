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
 * transitions.h
 *
 *  Created on: Jan 11, 2011
 *      Author: Ignacio Laguna
 *     Contact: ilaguna@purdue.edu
 *        Modified on: Nov 2, 2013
 *         *    Author: Subrata Mitra
 *          *    Contact: mitra4@purdue.edu
 *
 */
 

#ifndef TRANSITIONS_H_
#define TRANSITIONS_H_

//#if HAVE_CONFIG_H
#include "statetracker-config.h"
//#endif

#include "model_components.h"
#include "utilities.h"

#include<map>
#include<list>

class TransitionsManager {
private:
	State lastState;
	//SnapShot lastSnapShot;
	size_t numParametersToKeep;
    
	int cache_hit_count;
	map<State,bool> cache_Last_N_States;
	list<State> cache_Last_N_States_queue;

        TransitionsManager();	
	static State buildState ();
	void updateGlobalParamsTables(OperationType t, const MPIParams &p) const;
	void signalCondition();
public:
	//static SnapShot createSnapShot();
	TransitionsManager(const State &s) :
		lastState(s), numParametersToKeep(50), cache_hit_count(0) {};
	//TransitionsManager(const SnapShot &snap) :
	//	lastState(snap.state), lastSnapShot(snap), numParametersToKeep(50) {};
	void transitionBeforeMPICall(/*OperationType t, const MPIParams &p*/);
	void transitionAfterMPICall();
	State getLastState() const;
	bool checkCachedStates(State & currentState);
};

#endif /* TRANSITIONS_H_ */
