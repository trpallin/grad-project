#include <unistd.h>

#ifndef __GASAL_ALIGN_H__
#define __GASAL_ALIGN_H__
/*  ####################################################################################
    SEMI_GLOBAL Kernels generation - read from the bottom one, all the way up. (the most specialized ones are written before the ones that call them)
    ####################################################################################
*/
#define SEMIGLOBAL_KERNEL_CALL(a,s,h,t,b,m,g) \
	case t:\
		{\
		gasal_semi_global_kernel<Int2Type<a>, Int2Type<s>, Int2Type<b>, Int2Type<h>, Int2Type<t>><<<N_BLOCKS, BLOCKDIM, 0, gpu_storage->str>>>(gpu_storage->packed_query_batch, gpu_storage->packed_target_batch, gpu_storage->query_batch_lens, gpu_storage->target_batch_lens, gpu_storage->query_batch_offsets, gpu_storage->target_batch_offsets, gpu_storage->device_res, gpu_storage->device_res_second, gpu_storage->packed_tb_matrices, actual_n_alns);\
		break;\
		}\

#define SWITCH_SEMI_GLOBAL_TAIL(a,s,h,t,b,m,g) \
	case h:\
	switch(t) { \
		SEMIGLOBAL_KERNEL_CALL(a,s,h,NONE,b,m,g)\
		SEMIGLOBAL_KERNEL_CALL(a,s,h,QUERY,b,m,g)\
		SEMIGLOBAL_KERNEL_CALL(a,s,h,TARGET,b,m,g)\
		SEMIGLOBAL_KERNEL_CALL(a,s,h,BOTH,b,m,g)\
	}\
	break;

#define SWITCH_SEMI_GLOBAL_HEAD(a,s,h,t,b,m,g) \
	case s:\
	switch(h) { \
		SWITCH_SEMI_GLOBAL_TAIL(a,s,NONE,t,b,m,g)\
		SWITCH_SEMI_GLOBAL_TAIL(a,s,QUERY,t,b,m,g)\
		SWITCH_SEMI_GLOBAL_TAIL(a,s,TARGET,t,b,m,g)\
		SWITCH_SEMI_GLOBAL_TAIL(a,s,BOTH,t,b,m,g)\
	} \
	break;


/*  ####################################################################################
    ALGORITHMS Kernels generation. Allows to have a single line written for all kernels calls. The switch-cases are MACRO-generated.
    #################################################################################### 
*/

#define SWITCH_SEMI_GLOBAL(a,s,h,t,b,m,g) SWITCH_SEMI_GLOBAL_HEAD(a,s,h,t,b,m,g)

#define SWITCH_LOCAL(a,s,h,t,b,m,g) \
		case s: {\
			std::ofstream out;\
            out.open("/nfs/home/syeonp/SW/runtime/runtime.log", std::ios::app);\
            cudaEvent_t start, stop;\
            cudaEventCreate(&start);\
            cudaEventCreate(&stop);\
            cudaEventRecord(start);\
			gasal_local_kernel<Int2Type<LOCAL>, Int2Type<s>, Int2Type<b>><<<N_BLOCKS, BLOCKDIM, (BLOCKDIM/8)*512*sizeof(short2)+BLOCKDIM*3*sizeof(int32_t), gpu_storage->str>>>(gpu_storage->packed_query_batch, gpu_storage->packed_target_batch, gpu_storage->query_batch_lens, gpu_storage->target_batch_lens, gpu_storage->query_batch_offsets, gpu_storage->target_batch_offsets, gpu_storage->device_res, gpu_storage->device_res_second, gpu_storage->packed_tb_matrices, actual_n_alns, maximum_sequence_length, global_inter_row, NULL, NULL, NULL, NULL, NULL); \
			cudaDeviceSynchronize();\
            cudaEventRecord(stop);\
            cudaEventSynchronize(stop);\
            float mill = 0;\
            cudaEventElapsedTime(&mill, start, stop);\
            fprintf(stderr, "malloc time (in milliseconds): %.10f\n", mill);\
            out << mill;\
            out << std::endl;\
            out.close();\
            cudaEventDestroy(start);\
            cudaEventDestroy(stop);\
			break;\
		}\

#define DYNAMIC_TB
#define DBLOCK_SIZE 128

#ifdef DYNAMIC_TB

#define SWITCH_LOCAL_TB(a,s,h,t,b,m,g) \
		case s: {\
			gasal_local_kernel<Int2Type<LOCAL>, Int2Type<s>, Int2Type<b>><<<N_BLOCKS, BLOCKDIM, (BLOCKDIM/8)*512*sizeof(short2)+BLOCKDIM*3*sizeof(int32_t), gpu_storage->str>>>(gpu_storage->packed_query_batch, gpu_storage->packed_target_batch, gpu_storage->query_batch_lens, gpu_storage->target_batch_lens, gpu_storage->query_batch_offsets, gpu_storage->target_batch_offsets, gpu_storage->device_res, gpu_storage->device_res_second, gpu_storage->packed_tb_matrices, actual_n_alns, maximum_sequence_length, global_inter_row, global_direction, dblock_row, dblock_col, gpu_storage->dp_matrix_offsets, gpu_storage->global_direction_offsets); \
			cudaDeviceSynchronize();\
			cudaError_t aln_kernel_err = cudaGetLastError();\
			if ( cudaSuccess != aln_kernel_err )\
			{\
				fprintf(stderr, "[GASAL CUDA ERROR:] %s(CUDA error no.=%d). Line no. %d in file %s\n", cudaGetErrorString(aln_kernel_err), aln_kernel_err,  __LINE__, __FILE__);\
				exit(EXIT_FAILURE);\
			}\
			traceback_kernel_dynamic<<<N_BLOCKS, BLOCKDIM, BLOCKDIM*DBLOCK_SIZE*6, gpu_storage->str>>>(gpu_storage->unpacked_query_batch, gpu_storage->unpacked_target_batch, gpu_storage->query_batch_lens, gpu_storage->target_batch_lens, gpu_storage->query_batch_offsets, gpu_storage->target_batch_offsets, result_query, result_target, gpu_storage->device_res, actual_n_alns, maximum_sequence_length, dblock_row, dblock_col, gpu_storage->dp_matrix_offsets);\
			aln_kernel_err = cudaGetLastError();\
			if ( cudaSuccess != aln_kernel_err )\
			{\
				fprintf(stderr, "[GASAL CUDA ERROR:] %s(CUDA error no.=%d). Line no. %d in file %s\n", cudaGetErrorString(aln_kernel_err), aln_kernel_err,  __LINE__, __FILE__);\
				exit(EXIT_FAILURE);\
			}\
			break;\
		}\

#else

#define SWITCH_LOCAL_TB(a,s,h,t,b,m,g) \
		case s: {\
			gasal_local_kernel<Int2Type<LOCAL>, Int2Type<s>, Int2Type<b>><<<N_BLOCKS, BLOCKDIM, (BLOCKDIM/8)*512*sizeof(short2)+BLOCKDIM*3*sizeof(int32_t), gpu_storage->str>>>(gpu_storage->packed_query_batch, gpu_storage->packed_target_batch, gpu_storage->query_batch_lens, gpu_storage->target_batch_lens, gpu_storage->query_batch_offsets, gpu_storage->target_batch_offsets, gpu_storage->device_res, gpu_storage->device_res_second, gpu_storage->packed_tb_matrices, actual_n_alns, maximum_sequence_length, global_inter_row, global_direction, dblock_row, dblock_col, gpu_storage->dp_matrix_offsets, gpu_storage->global_direction_offsets); \
			cudaDeviceSynchronize();\
			traceback_kernel_old<<<N_BLOCKS, BLOCKDIM, 0, gpu_storage->str>>>(gpu_storage->unpacked_query_batch, gpu_storage->unpacked_target_batch, gpu_storage->query_batch_lens, gpu_storage->target_batch_lens, gpu_storage->query_batch_offsets, gpu_storage->target_batch_offsets, global_direction, result_query, result_target, gpu_storage->device_res, actual_n_alns, maximum_sequence_length, gpu_storage->global_direction_offsets);\
			cudaError_t aln_kernel_err = cudaGetLastError();\
			if ( cudaSuccess != aln_kernel_err )\
			{\
				fprintf(stderr, "[GASAL CUDA ERROR:] %s(CUDA error no.=%d). Line no. %d in file %s\n", cudaGetErrorString(aln_kernel_err), aln_kernel_err,  __LINE__, __FILE__);\
				exit(EXIT_FAILURE);\
			}\
			break;\
		}\

#endif


	

#define SWITCH_GLOBAL(a,s,h,t,b,m,g) \
		case s:{\
			gasal_global_kernel<Int2Type<s>><<<N_BLOCKS, BLOCKDIM, 0, gpu_storage->str>>>(gpu_storage->packed_query_batch, gpu_storage->packed_target_batch, gpu_storage->query_batch_lens,gpu_storage->target_batch_lens, gpu_storage->query_batch_offsets, gpu_storage->target_batch_offsets, gpu_storage->device_res, gpu_storage->packed_tb_matrices, actual_n_alns);\
			if(s == WITH_TB) {\
				cudaError_t aln_kernel_err = cudaGetLastError();\
				if ( cudaSuccess != aln_kernel_err )\
				{\
					fprintf(stderr, "[GASAL CUDA ERROR:] %s(CUDA error no.=%d). Line no. %d in file %s\n", cudaGetErrorString(aln_kernel_err), aln_kernel_err,  __LINE__, __FILE__);\
					exit(EXIT_FAILURE);\
				}\
				gasal_get_tb<Int2Type<GLOBAL>><<<N_BLOCKS, BLOCKDIM, 0, gpu_storage->str>>>(gpu_storage->unpacked_query_batch, gpu_storage->query_batch_lens, gpu_storage->target_batch_lens, gpu_storage->query_batch_offsets, gpu_storage->packed_tb_matrices, gpu_storage->device_res, gpu_storage->, maximum_sequence_length, global_inter_row);\
			}\
			break;\
		}\


#define SWITCH_KSW(a,s,h,t,b,m,g) \
    case s:\
        gasal_ksw_kernel<Int2Type<b>><<<N_BLOCKS, BLOCKDIM, 0, gpu_storage->str>>>(gpu_storage->packed_query_batch, gpu_storage->packed_target_batch, gpu_storage->query_batch_lens, gpu_storage->target_batch_lens, gpu_storage->query_batch_offsets, gpu_storage->target_batch_offsets, gpu_storage->seed_scores, gpu_storage->device_res, gpu_storage->device_res_second, actual_n_alns);\
    break;

#define SWITCH_BANDED(a,s,h,t,b,m,g) \
    case s:\
        gasal_banded_tiled_kernel<<<N_BLOCKS, BLOCKDIM, 0, gpu_storage->str>>>(gpu_storage->packed_query_batch, gpu_storage->packed_target_batch, gpu_storage->query_batch_lens,gpu_storage->target_batch_lens, gpu_storage->query_batch_offsets, gpu_storage->target_batch_offsets, gpu_storage->device_res, actual_n_alns,  k_band>>3); \
break;

/*  ####################################################################################
    RUN PARAMETERS calls : general call (bottom, should be used), and first level TRUE/FALSE calculation for second best, 
    then 2nd level WITH / WITHOUT_START switch call (top)
    ####################################################################################
*/

#define SWITCH_START(a,s,h,t,b,m,g) \
    case b: \
    switch(s){\
        SWITCH_## a(a,WITH_START,h,t,b,m,g)\
        SWITCH_## a(a,WITHOUT_START,h,t,b,m,g)\
        SWITCH_## a(a,WITH_TB,h,t,b,m,g)\
    } \
    break;

#define SWITCH_SECONDBEST(a,s,h,t,b,m,g) \
    switch(b) { \
        SWITCH_START(a,s,h,t,TRUE,m,g)\
        SWITCH_START(a,s,h,t,FALSE,m,g)\
    }

#define KERNEL_SWITCH(a,s,h,t,b,m,g) \
    case a:\
        SWITCH_SECONDBEST(a,s,h,t,b,m,g)\
    break;

#define KERNEL_SWITCH_LOCAL(a,s,h,t,b,m,g) \
    case a:\
        switch(b) { \
			case TRUE: \
				switch(s){\
					SWITCH_LOCAL_TB(a,WITH_START,h,t,TRUE,m,g)\
					SWITCH_LOCAL_TB(a,WITHOUT_START,h,t,TRUE,m,g)\
					SWITCH_LOCAL_TB(a,WITH_TB,h,t,TRUE,m,g)\
				} \
				break;\
			case FALSE: \
				switch(s){\
					SWITCH_LOCAL_TB(a,WITH_START,h,t,FALSE,m,g)\
					SWITCH_LOCAL_TB(a,WITHOUT_START,h,t,FALSE,m,g)\
					SWITCH_LOCAL_TB(a,WITH_TB,h,t,FALSE,m,g)\
				} \
				break;\
		}\
    break;


/* // Deprecated
void gasal_aln(gasal_gpu_storage_t *gpu_storage, const uint8_t *query_batch, const uint32_t *query_batch_offsets, const uint32_t *query_batch_lens, const uint8_t *target_batch, const uint32_t *target_batch_offsets, const uint32_t *target_batch_lens,   const uint32_t actual_query_batch_bytes, const uint32_t actual_target_batch_bytes, const uint32_t actual_n_alns, int32_t *host_aln_score, int32_t *host_query_batch_start, int32_t *host_target_batch_start, int32_t *host_query_batch_end, int32_t *host_target_batch_end,  algo_type algo, comp_start start, int32_t k_band);
*/

void gasal_copy_subst_scores(gasal_subst_scores *subst);

void gasal_aln_async(gasal_gpu_storage_t *gpu_storage, const uint32_t actual_query_batch_bytes, const uint32_t actual_target_batch_bytes, const uint32_t actual_n_alns, Parameters *params, uint32_t maximum_sequence_length, short2* global_inter_row, uint32_t* global_direction, uint8_t *result_query, uint8_t *result_target, short2 *dblock_row, short2 *dblock_col);

inline void gasal_kernel_launcher(int32_t N_BLOCKS, int32_t BLOCKDIM, algo_type algo, comp_start start, gasal_gpu_storage_t *gpu_storage, int32_t actual_n_alns, int32_t k_band, uint32_t maximum_sequence_length, short2* global_inter_row);

int gasal_is_aln_async_done(gasal_gpu_storage_t *gpu_storage);

#endif
