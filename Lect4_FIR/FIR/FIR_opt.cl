/*
 * Calculate a FIR filter
 * one work group, and the number of work items is the number of output points
 */

__kernel void FIR( __global float * output,
                   __global float * coeff,         /* numFilter sets of coeff, each is numTap long */
                   __global float * temp_input,    /* numFilter sets of temp data, each 
                                                   is (numData+numTap-1) long, 
                                                   the first (numTap-1) data is actually the history data */
                   uint numTap){

    uint tid = get_global_id(0);
    uint lid = get_local_id(0);
    
    uint numData = get_global_size(0);
    
    uint xid = tid + numTap - 1;
    
    //Use Local memory
    __local float l_coeff[numTap];
    
    float sum = 0;
    uint i=0;

    l_coeff[lid] = coeff[lid];
    barrier(CLK_LOCAL_MEM_FENCE);
    
    for( i=0; i<numTap; i++ )
    {
        sum += l_coeff[i] * temp_input[tid + i];
    }
    output[tid] = sum;


    /* fill the history buffer for multi block computation */
    //if( tid >= numData - numTap + 1 )
    //    temp_input[ tid - ( numData - numTap + 1 ) ] = temp_input[xid];

    //barrier( CLK_GLOBAL_MEM_FENCE );
}
