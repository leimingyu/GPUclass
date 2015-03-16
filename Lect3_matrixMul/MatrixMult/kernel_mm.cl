__kernel void MatrixMult_naive(__global float *A,
		                       __global float *B,
		                       __global float *C,
		                       const int wA,
							   const int wB)
{
	uint col = get_global_id(0);
	uint row = get_global_id(1);

	float sum = 0.f;

	int k;
	for(k=0; k<wA; ++k)
	{
		sum += A[row * wA + k] * B[k * wB + col];	
	}

	C[row * wB + col] = sum;
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


	// start index of sub-matrix in A
	int aBegin = wA * 16 * by;

	// end index of sub-matix in A
	int aEnd = aBegin + wA - 1;

	// step size of iteration in A
	int aStep = 16;

	// start index of submatrix in B
	int bBegin = 16 * bx;

	// step size of iteration in B
	int bStep = 16 * wB;

	// dot product
	float sum = 0.f;

	int a, b;
	for(a = aBegin, b = bBegin; a <= aEnd; a += aStep, b += bStep)
	{
		// load titled sub-matrices into local memory	

		sma[ty * 16 + tx] = A[a + ty * wA + tx];
		smb[ty * 16 + tx] = B[b + ty * wB + tx];

		barrier(CLK_LOCAL_MEM_FENCE);

		int k;
		#pragma unroll
		for(k = 0; k < 16; ++k)
		{
			sum += sma[ty * 16 + k] * smb[k * 16 + tx];
		}

		barrier(CLK_LOCAL_MEM_FENCE);
	}

	uint index_out = wB * 16 * by + wB * ty + 16 * bx + tx;
	C[index_out] = sum;
}
