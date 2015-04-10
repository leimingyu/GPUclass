
__kernel void matrix_transpose(
							   __global float *A,
							   __global float *At,
							   const int hA,
							   const int wA)
{
	// write your code here

	__local float lds[256]; // 16 x 16

	size_t lix = get_local_id(0); // col thread
	size_t liy = get_local_id(1); // row thread

	size_t bx = get_group_id(0); // col block
	size_t by = get_group_id(1); // row block

	// reshuffle blocks to cacluate the input block id
	size_t bx_i = by;
	size_t by_i = bx;

	// index the reading data
	// A[idy][idx]
	size_t idx = bx_i * 16 + lix;
	size_t idy = by_i * 16 + liy;
	size_t index_in = idy * wA + idx; 

	// local memory: [liy][lix]
	size_t ind_local = liy * 16 + lix; 

	// copy from global memory to LDS
	lds[ind_local] = A[index_in];

	barrier(CLK_LOCAL_MEM_FENCE);

	// the output index
	idx = bx * 16 + lix;
	idy = by * 16 + liy;
	int index_out = idy * hA + idx;

	// transpose inside LDS
	// [lix][liy]
	ind_local = lix * 16 + liy; 

	// output
	At[index_out] = lds[ind_local];
}

