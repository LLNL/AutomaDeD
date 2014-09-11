AutomaDeD
=========

Prodometer
----------
This version of AutomaDeD implements the diagnosis algorithm of the [Prodometer](http://dx.doi.org/10.1145/2666356.2594336) technique, which performs loop-aware progress-dependence analysis. For more information, please refer to [Prodometer](http://dx.doi.org/10.1145/2666356.2594336).

1. Description
--------------
AutomaDeD (Automata-based Debugging for Dissimilar parallel tasks) is a tool for automatic diagnosis of performance and correctness problems in MPI applications. It creates control-flow models of each MPI process and, when a failure occurs, these models are leveraged to find the origin of problems automatically. MPI calls are intercepted (using wrappers) to create the models. When an MPI application hangs, AutomaDeD creates a progress-dependence graph that helps finding the process (or group of processes) that caused the hang. Please refer to [2] for more details.

                        
2. Building
-----------

For Unix-based machines (with Cmake), simply execute:
	
	$ cmake -DCMAKE_INSTALL_PREFIX=<install_path>

To use callpath library(to normalize the library loading order):

	$ cmake -DCMAKE_INSTALL_PREFIX=<install_path> -DSTATE_TRACKER_WITH_CALLPATH=ON

	This will require two additional libraries callpath and adept_utils. You can get those from the following link:
	https://github.com/scalability-llnl.

Then:

	$ make
	$ make install

It requires a C++ MPI compiler wrapper (like mpic++) The configure script should detect automatically your MPI compiler installation. If you want to specify a particular compiler, it can be done standard CMake techniques. 


BOOST should be installed in your system. CMake will try to detect boost in your system. To set the path for BOOST for CMake to find, please use: -D BOOST_ROOT=<BOOST base dir>.

3. Running
----------

You have to link your MPI application against AutomaDeD's library. This could be done using either the static or the shared library. Once this is done, you can run your buggy application. You can use LD_PRELOAD=<install>/lib/libstracker.so srun -n 16 -ppdebug ./test to run test application.

Take a look at the './example' directory to see some use cases.

To run with callpath library, please set env variable: AUT_USE_CALL_PATH=TRUE

You can stop dumping the tool output file using: export AUT_DO_NOT_DUMP=TRUE

If you choose to attach other debuggers on the LP process identified by the tool, you can use: export AUT_DO_NOT_EXIT=TRUE, to make sure the tool does not exit 

4. About BG/Q systems
---------------------
For BG/Q system, you need to specify Toolchain file for CMake: -D CMAKE_TOOLCHAIN_FILE=cmakemodules/Toolchain/BlueGeneQ-gnu.cmake

5. Using the GUI
----------------
AutomaDeD comes with a GUI which can read the AUT* file generated by the tool. The GUI, <pdgview> has a documentation file which explains how to use the GUI.

6. Known issues
---------------
If callpath is used, currently it does not give the full file name and line number information in the output file. So GUI can not be used. This support will be added soon.

7. References
-------------
[1] Subrata Mitra, Ignacio Laguna, Dong H. Ahn, Saurabh Bagchi, Martin Schulz, 
Todd Gamblin, "Accurate application progress analysis for large-scale parallel 
debugging", ACM SIGPLAN Conference on Programming Language Design and 
Implementation (PLDI), 2014.

[2] Ignacio Laguna, Dong H. Ahn, Bronis R. de Supinski, Saurabh Bagchi, Todd 
Gamblin, "Probabilistic Diagnosis of Performance Faults in Large-Scale Parallel 
Applications", International Conference on Parallel Architectures and 
Compilation Techniques (PACT), 2012.

--------------------------------------------------------------------------------

Authors
-------

The main code infrastructure of AutomaDeD was written by: Ignacio Laguna (ilaguna@llnl.gov), LLNL

The code that implements the Prodometer algorithms was written by: Subrata Mitra (mitra4@purdue.edu), Purdue University

Project contributors: \\
Dong H. Ahn (LLNL) \\
Saurabh Bagchi (Purdue University) \\
Bronis R. de Supinski (LLNL) \\
Todd Gamblin (LLNL) \\
Martin Schulz (LLNL) \\

--------------------------------------------------------------------------------

