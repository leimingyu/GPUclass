#define TILE 16

__kernel void transpose(
		__global float *A,
		__global float *At,
		__local  float *lds )
{
	// square matrix
	size_t N  = get_global_size(0);

	size_t gx = get_group_id(0);
	size_t gy = get_group_id(1);

	size_t blks_x = get_num_groups(0);

	// reshuffle blocks
	size_t giy = gx;
	size_t gix = (gx + gy)%blks_x;

	size_t lix = get_local_id(0);
	size_t liy = get_local_id(1);

	// use reshuffled blks to index the reading data
	size_t ix = gix * TILE + lix;
	size_t iy = giy * TILE + liy;

	size_t index_in = ix + iy * N; // [iy][ix]

	// copy from global memory to LDS
	size_t ind = liy * TILE  + lix; //[liy][lix]

	lds[ind]            =   A[index_in];

	barrier(CLK_LOCAL_MEM_FENCE);

	ix = giy * TILE + lix;
	iy = gix * TILE + liy;

	// transpose the index inside LDS
	ind = lix * TILE  + liy; // [lix][liy]

	int index_out = ix  + iy * N ;

	At[index_out]           = lds[ind];
}

