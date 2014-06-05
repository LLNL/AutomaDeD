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
 * io_utilities.h
 *
 *  Created on: Jun 16, 2013
 *      Author: Ignacio Laguna
 *     Contact: ilaguna@llnl.gov
 *        Modified on: Nov 2, 2013
 *         *    Author: Subrata Mitra
 *          *    Contact: mitra4@purdue.edu
 *
 */

#ifndef IO_UTILITIES_H_
#define IO_UTILITIES_H_

#include "markov_model.h"
#include "model_components.h"

#include <string>
#include <vector>
#include "graph_model.h"

using namespace std;

void getUniqueFileName(char *fileName, const char *extension);
string writeFile(const string &data, const char *extension);
void dumpMarkovModel(MarkovModel<State>* mm);
void dumpStateFactory(StateFactory& sf);
void dumpStateFactoryWithResolvedName(StateFactory& sf);
void dumpGraph(Graph_Edges* ge);
void dumpMarkovModelPerRankAsCSV(MarkovModel<State>* mm);
/**
 * Returns all output lines of the command
 */
vector<string> executeShellCommand(const string &command);

#endif /* IO_UTILITIES_H_ */
