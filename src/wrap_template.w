#include <iostream>

/* Helper functions prototypes */
void beforeMPICall(const char *functionName,...);
void afterMPICall(const char *functionName);
void afterInit(int *argc, char ***argv);
void beforeFinalize(void);
void sigterm_handler(int sig_number);

{{fnall foo MPI_Init MPI_Init_thread MPI_Finalize MPI_Abort MPI_Accumulate MPI_Add_error_class MPI_Add_error_code MPI_Add_error_string MPI_Alloc_mem MPI_Alltoallw MPI_Close_port MPI_Comm_accept MPI_Comm_call_errhandler MPI_Comm_connect MPI_Comm_create_errhandler MPI_Comm_create_keyval MPI_Comm_delete_attr MPI_Comm_disconnect MPI_Comm_free_keyval MPI_Comm_get_attr MPI_Comm_get_errhandler MPI_Comm_get_parent MPI_Comm_join MPI_Comm_set_attr MPI_Comm_set_errhandler MPI_Comm_spawn MPI_Comm_spawn_multiple MPI_Dist_graph_create MPI_Dist_graph_create_adjacent MPI_Dist_graph_neighbors MPI_Dist_graph_neighbors_count MPI_Exscan MPI_File_call_errhandler MPI_File_create_errhandler MPI_Free_mem MPI_Get MPI_Get_address MPI_Grequest_complete MPI_Grequest_start MPI_Is_thread_main MPI_Lookup_name MPI_Op_commutative MPI_Open_port MPI_Pack_external MPI_Pack_external_size MPI_Publish_name MPI_Put MPI_Query_thread MPI_Reduce_local MPI_Reduce_scatter_block MPI_Request_get_status MPI_Type_create_f90_complex MPI_Type_create_f90_integer MPI_Type_create_f90_real MPI_Type_create_hindexed MPI_Type_create_hvector MPI_Type_create_keyval MPI_Type_create_resized MPI_Type_create_struct MPI_Type_delete_attr MPI_Type_dup MPI_Type_free_keyval MPI_Type_get_attr MPI_Type_get_extent MPI_Type_get_name MPI_Type_get_true_extent MPI_Type_match_size MPI_Type_set_attr MPI_Type_set_name MPI_Unpack_external MPI_Unpublish_name MPI_Win_call_errhandler MPI_Win_complete MPI_Win_create MPI_Win_create_errhandler MPI_Win_create_keyval MPI_Win_delete_attr MPI_Win_fence MPI_Win_free MPI_Win_free_keyval MPI_Win_get_attr MPI_Win_get_errhandler MPI_Win_get_group MPI_Win_get_name MPI_Win_lock MPI_Win_post MPI_Win_set_attr MPI_Win_set_errhandler MPI_Win_set_name MPI_Win_start MPI_Win_test MPI_Win_unlock MPI_Win_wait MPI_Wtime MPI_Wtick}} 
    beforeMPICall("{{foo}}", {{list {{args}}}});
    {{callfn}}
    afterMPICall("{{foo}}");
{{endfnall}}

{{fn foo MPI_Init}}
    int rank;
    int required=MPI_THREAD_MULTIPLE, provided;
    MPI_Comm comm = MPI_COMM_WORLD;
   
    {{ret_val}} = PMPI_Init_thread({{list {{args}}}}, required, &provided);
    PMPI_Comm_rank(comm,&rank);
    if (rank == 0) {
        std::cout << "===============================" << std::endl;
        std::cout << "AutomaDeD started in MPI_Init" << std::endl;
        std::cout << "Tool: statetracker" << std::endl;
        std::cout << "By Ignacio Laguna et al.\n" << std::endl;

        std::cout << "MPI_INIT_THREAD Level" << std::endl;
        std::cout << "Required: " << required << std::endl;
        std::cout << "Provided: " << provided << std::endl;
        std::cout << "===============================" << std::endl;
    }

    if ({{ret_val}} == MPI_SUCCESS) {
       afterInit({{args 0}} , {{args 1}});
    }
{{endfn}}

{{fn foo MPI_Init_thread}}
    int rank;
    {{args 2}} = {{args 2}} < MPI_THREAD_MULTIPLE ? MPI_THREAD_MULTIPLE : {{args 2}};
    MPI_Comm comm = MPI_COMM_WORLD;
   
    {{callfn}}
    
    PMPI_Comm_rank(comm,&rank);
    if (rank == 0) {
	std::cout << "*** FSM started in MPI_Init_thread ***" << std::endl;
	std::cout << "Required: " << {{args 2}} << " Provided: " << {{args 3}} << std::endl;
    }

    if ({{ret_val}} == MPI_SUCCESS) {
       afterInit({{args 0}} , {{args 1}});
    }
{{endfn}}

{{fn foo MPI_Finalize}}
    beforeFinalize();
    {{callfn}}
{{endfn}}

{{fn foo MPI_Abort}}
    sigterm_handler(3);
{{endfn}}

