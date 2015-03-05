__kernel void VectorAddKernel(__global float *c,
		__global float *a,
		__global float *b,
		const unsigned int n)
{
	size_t wid = get_global_id(0);

	c[wid] = a[wid] + b[wid];
}

