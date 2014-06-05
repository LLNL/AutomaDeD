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
 * helper_thread.C
 *
 *  Created on: Mar 6, 2012
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

#include "helper_thread.h"
#include "model_components.h"
#include "debugging.h"
#include "mpi_state.h"
#include "markov_model.h"
#include "analysis_driver.h"
#include "config.h"

#include <pthread.h>
#include <assert.h>
#include <errno.h>
#include <signal.h>
#include <time.h>
#include <sys/time.h>
#include <mpi.h>

#include <iostream>

using namespace std;

extern StateFactory *sFactory;
extern MPIState mpiState;
extern MarkovModel<State> markovModel;
void initHelperThread(pthread_t *t, ThreadData *d)
{
	// Initialize mutex and condition variable
	d->mutex = (pthread_mutex_t *)xmalloc(sizeof (pthread_mutex_t));
	pthread_mutex_init(d->mutex, NULL);
	d->condition = (pthread_cond_t *) xmalloc(sizeof (pthread_cond_t));
	pthread_cond_init(d->condition, NULL);

	// Initialize helper thread.
	int ret = pthread_create(t, NULL, helperThread, static_cast<void *>(d));

#if STATE_TRACKER_DEBUG
	if (ret != 0)
		handleError("in initHelperThread: Error creating thread!");
#endif
}

void * helperThread(void *par)
{
	ThreadData *threadData = static_cast<ThreadData *>(par);
	unsigned int transTime = 60; // transition waiting time
        int timeout = 60;
        bool r = AUTConfig::getIntParameter("AUT_TIMEOUT", timeout);
        if(r)
        {       if (mpiState.isRoot())
                {
                  cout << "Timeout is= " << timeout << endl;
                }
                transTime = timeout;
        }
        bool noBarrier = false;
        r = AUTConfig::getBoolParameter("AUT_NO_BARRIER", noBarrier);

        bool donotCopyStateFactory = false;
	r = AUTConfig::getBoolParameter("AUT_DO_NOT_NOT_COPY_STATEFACTORY", donotCopyStateFactory);
	if (mpiState.isRoot())
	{
		if(noBarrier)
		{
			cout << "Will not call any Barrier" << endl;
		}
		if(donotCopyStateFactory)
		{
			cout << "will not copy statefactory" << endl;
		}
	
	}

	//QueueMsg *message;
	//time_t lastSnapshot = time(NULL);
	time_t lastTransition = time(NULL); // Last time a transition was observed
	struct timeval currentTime;
	struct timespec expireTime;
	int ret;

	while(1) {
		assert(pthread_mutex_lock(threadData->mutex) == 0); // Acquire lock

		gettimeofday(&currentTime, NULL);
		expireTime.tv_sec = currentTime.tv_sec + 3;
		expireTime.tv_nsec = currentTime.tv_usec * 1000;

		ret = 0;
		while (1) {
			//cout << lastTransition << endl;
			ret = pthread_cond_timedwait(threadData->condition,
					threadData->mutex, &expireTime);

//#if STATE_TRACKER_DEBUG
			if (ret != ETIMEDOUT && ret != 0)
				handleError("in helperThread(): "
						"problem in thread when waiting for event");
//#endif

			if (ret == 0) // we have seen a transition
			{
				break;

			}

			// Check for a long time between a transition.
			// This is to handle the a hang (infinite loop) case.

			if (time(NULL) >=
					(lastTransition + static_cast<time_t>(transTime))) {
				// Release lock
				//assert(pthread_mutex_unlock(threadData->mutex) == 0);

				/*-------------------------------------------------------*/
				/* Analysis												 */
				/*-------------------------------------------------------*/

				// first copy the markov model at this state
				MarkovModel<State>* copy = new MarkovModel<State>(); 

				//TODO: check which behavior should we use by deafult, Do we need to copy to prevent crash?
				//What are the chances? Disabling copy for now with the following line...for performance.. 
				//But, we have seen a crash in one weired example.
				donotCopyStateFactory = true;
				StateFactory* copyFactory = new StateFactory();
				if(!donotCopyStateFactory)
				{
				  markovModel.copyMarkovModel(copy);
				  sFactory->copyStateFactory(copyFactory);
				}

				//cout << "Copied everything" << endl;
                                if(!noBarrier)
                                {
				  PMPI_Barrier(mpiState.getWorldComm());
				}
	                        int rank = mpiState.getProcessRank();


				//AnalysisDriver driver(mpiState, &markovModel, sFactory);
				if(!donotCopyStateFactory)
		                {
				   AnalysisDriver driver(mpiState, copy, copyFactory);
				   driver.findLeastProgressedTasks(true);
				}
				else
				{
				    AnalysisDriver driver(mpiState, &markovModel, sFactory);
				    driver.findLeastProgressedTasks(true);
				}
                                if(!noBarrier)
				{
				  PMPI_Barrier(mpiState.getWorldComm());
				}
				/*-------------------------------------------------------*/
				bool doNotExit = false;
				bool r = false;
                                r = AUTConfig::getBoolParameter("AUT_DO_NOT_EXIT", doNotExit);
				if(!r || !doNotExit)
				{
				  exit(EXIT_SUCCESS);
				}
				else
				{
					sleep(60*60*60);
					//do not exit
				}
			}
		}

		// Save last transition time
		lastTransition = time(NULL);

		assert(pthread_mutex_unlock(threadData->mutex) == 0); // Release lock
	}

	pthread_exit(NULL);
}
