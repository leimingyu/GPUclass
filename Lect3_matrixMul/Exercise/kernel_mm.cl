__kernel void MatrixMult_naive(__global float *A,
		                       __global float *B,
		                       __global float *C,
		                       const int wA,
							   const int wB)
{
	uint col = get_global_id(0);
	uint row = get_global_id(1);

	// write your code here
}

__kernel void MatrixMult_opt(__global float *A,
		                     __global float *B,
		                     __global float *C,
		                     const int wA,
							 const int wB)
{
	__local float sma[256]; // 16 x 16
	__local float smb[256];

	int bx = get_group_id(0); // col
	int by = get_group_id(1); // row

	int tx = get_local_id(0);
	int ty = get_local_id(1);

	// write your code here

}
