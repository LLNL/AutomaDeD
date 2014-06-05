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
 * transitions.cxx
 *
 *  Created on: Jan 11, 2011
 *      Author: Ignacio Laguna
 *     Contact: ilaguna@purdue.edu
 */

//#if HAVE_CONFIG_H
#include "statetracker-config.h"
//#endif

#include "transitions.h"
#include "debugging.h"
#include "model_components.h"
#include "backtrace.h"
#include "utilities.h"
#include "helper_thread.h"
#include "markov_model.h"

#include <pthread.h>
#include <assert.h>
#include <string>
#include <map>
#include <list>

#define MAX_CACHE_HIT_TOLERANCE 10000
#define MAX_CACHE_LENGTH 20
using namespace std;

extern MarkovModel<State> markovModel;
extern StateFactory *sFactory;
//extern list< MPIParams > sentParams;
//extern list< MPIParams > receivedParams;
extern ThreadData threadData;

// return false => hang
// return true -> ok can signal progress
bool TransitionsManager::checkCachedStates(State & currentState)
{
	return true;
	
	//cout << "stateid: " << currentState.getId() << " cache size: " << cache_Last_N_States_queue.size() << " cache_hit_count: " << cache_hit_count << endl; 
	map<State,bool> :: iterator pos = cache_Last_N_States.find(currentState);
	if(pos != cache_Last_N_States.end())
	{
		if(cache_hit_count > MAX_CACHE_HIT_TOLERANCE)
		{
			return false;
		}
		else
		{
			cache_hit_count++;
			return true;
		}
	}
	else
	{
		cache_hit_count = 0;

		if(cache_Last_N_States_queue.size() > MAX_CACHE_LENGTH)
		{
			//delete the oldest state in LRU scheme
			State poppedState = cache_Last_N_States_queue.front();
			cache_Last_N_States_queue.pop_front();
			pos = cache_Last_N_States.find(poppedState);
			cache_Last_N_States.erase(pos);
		
			//add the new state
			cache_Last_N_States_queue.push_back(currentState);
			cache_Last_N_States.insert(make_pair(currentState,true));

			return true;

		}
		else
		{
                    //add the new state
		    cache_Last_N_States_queue.push_back(currentState);
		    cache_Last_N_States.insert(make_pair(currentState,true));
		    
		    return true;

		}
	}
}


void TransitionsManager::transitionBeforeMPICall(/*OperationType t,const MPIParams &p */)
{
	State newState = buildState();
	//SnapShot newSnapShot = createSnapShot();
	
	EdgeAnnotation edgeAnnot(1);
	//EdgeAnnotation edgeAnnot = EdgeAnnotation::createEdgeAnnotation(lastSnapShot, newSnapShot);
	// Update Markov Model
	markovModel.addEdge(lastState, newState,edgeAnnot);

	//Edge e(lastState, newState);
	lastState = newState;

	//lastSnapShot = newSnapShot;
	//lastState = newSnapShot.state;
	//fsm.addEdge(e);

	//MPIParams params = p;
	//params.state = newState.getString();
	//updateGlobalParamsTables(t, params);

	if(checkCachedStates(newState))
	{
	  signalCondition();
	}
}

/*void TransitionsManager::updateGlobalParamsTables(OperationType t,
		const MPIParams &p) const
{
	if (t == SEND_OP) {
		sentParams.push_back(p);
		if (sentParams.size() >= numParametersToKeep)
			sentParams.erase(sentParams.begin());
	} else if (t == RECV_OP) {
		receivedParams.push_back(p);
		if (receivedParams.size() >= numParametersToKeep)
			receivedParams.erase(receivedParams.begin());
	}
}*/

void TransitionsManager::transitionAfterMPICall()
{
	State newState = buildState();
	//SnapShot newSnapShot = createSnapShot();

	//EdgeAnnotation edgeAnnot = EdgeAnnotation::createEdgeAnnotation(lastSnapShot, newSnapShot);
	EdgeAnnotation edgeAnnot(1);

	// Update Markov Model
	markovModel.addEdge(lastState, newState,edgeAnnot);
	//markovModel.addEdge(lastSnapShot.state, newSnapShot.state,edgeAnnot);

	//Edge e(lastState, newState);
	lastState = newState;
	
	//lastSnapShot = newSnapShot;
 	//lastState = newSnapShot.state;
	//fsm.addEdge(e);

	if(checkCachedStates(newState))
	{
	  signalCondition();
	}
}
#if 0
SnapShot TransitionsManager::createSnapShot()
{
     State newState = buildState();
     //unsigned long newMemory = processMemoryTracker->getCurrentAllocation();
     unsigned long newMemory = 0;
     SnapShot snap(newState,newMemory);
     //snap.state = newState;
     //snap.memory = newMemory;
     //snap.time  = 0;

     return snap;
}
#endif
State TransitionsManager::buildState()
{
	string bt = Backtrace::getBacktrace();
	State s = sFactory->createState(bt);
	return s;
}


State TransitionsManager::getLastState() const
{
	return lastState;
}

void TransitionsManager::signalCondition()
{
	//assert(pthread_mutex_lock(threadData.mutex) == 0);
	pthread_cond_signal(threadData.condition);
	//assert(pthread_mutex_unlock(threadData.mutex) == 0);
}
