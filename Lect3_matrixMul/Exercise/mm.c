#include <CL/cl.h>

#include <assert.h>
#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>



/*
 * \brief Get platform.
 */
int GetPlatform(cl_platform_id *platform)
{
	// Get the number of platforms
	cl_uint num_platform = 0;
	cl_int status = clGetPlatformIDs(0, NULL, &num_platform);
	assert(status == CL_SUCCESS);
	assert(num_platform > 0);

	// Get all the available platforms
	cl_platform_id *platforms = (cl_platform_id *) 
		calloc(num_platform, sizeof(cl_platform_id));
	assert(platforms);
	status = clGetPlatformIDs(num_platform, platforms, NULL);
	assert(status == CL_SUCCESS);
	assert(platforms);

	// Choose the AMD platform
	for (cl_uint i = 0; i < num_platform; ++i)
	{
		char buf[100];
		printf("\nPlatform %d\n", i);

		status = clGetPlatformInfo(
				platforms[i],
				CL_PLATFORM_VENDOR,
				sizeof(buf),
				buf,
				NULL);
		assert(status == CL_SUCCESS);
		printf("CL_PLATFORM_VENDOR: %s\n", buf);

		if (!strncmp(buf, "Advanced Micro Devices, Inc.", 100))
		{
			*platform = platforms[i];
			break;
		}

	}
	free(platforms);

	assert(*platform != NULL);
	return 0;
}

/*
 * \brief Create context.
 */
int CreateContext(cl_context *ctx, cl_platform_id platform)
{
	// Set up context properties
	cl_context_properties prop[3] = {CL_CONTEXT_PLATFORM, 
		(cl_context_properties)platform, 
		0};

	// Create a context with all the available GPUs
	cl_int status = 0;
	*ctx = clCreateContextFromType(prop, 
			CL_DEVICE_TYPE_GPU, 
			NULL, 
			NULL, 
			&status);
	assert(status == CL_SUCCESS);

	assert(*ctx != NULL);
	return 0;
}

/*
 * \brief Get device available in the given context.
 */
int GetDevice(cl_device_id *dev, cl_context ctx)
{
	// Get the number of devices in a given context
	size_t num_device = 0;
	cl_int status = clGetContextInfo(
			ctx, 
			CL_CONTEXT_DEVICES, 
			0, 
			NULL, 
			&num_device);
	assert(status == CL_SUCCESS);
	assert(num_device > 0);

	// Get the first device
	status = clGetContextInfo(
			ctx, 
			CL_CONTEXT_DEVICES, 
			sizeof(cl_device_id), 
			dev, 
			NULL);
	assert(status == CL_SUCCESS);

	assert(*dev != NULL);
	return 0;
}

/*
 * \brief Create command queue for the given device.
 */
int CreateCommandQueue(cl_command_queue *cmd_q, cl_context ctx, 
		cl_device_id dev)
{
	cl_int status = 0;
	*cmd_q = clCreateCommandQueue(
			ctx, 
			dev, 
			0, 
			&status);
	assert(status == CL_SUCCESS);

	assert(*cmd_q != NULL);
	return 0;
}

/*
 * \brief Create and build program.
 */
int CreateProgram(cl_program *program, cl_context ctx, cl_device_id dev)
{
	// Convert the contents of a file into a string
	const char *filename  = "kernel_mm.cl";
	FILE *f = fopen(filename, "r");
	assert(f);
	fseek(f, 0, SEEK_END);
	size_t file_size = ftell(f);

	size_t src_size = file_size + 1;
	char *src = (char *) calloc(src_size, sizeof(char));
	assert(src);
	fseek(f, 0, SEEK_SET);
	fread(src, sizeof(char), file_size, f);
	fclose(f);

	// Create a program
	const char *srcs[] = {src};
	const size_t src_sizes[] = {src_size};
	cl_int status = 0;
	*program = clCreateProgramWithSource(
			ctx, 
			1, 
			srcs,
			src_sizes,
			&status);
	assert(status == CL_SUCCESS);

	// Build the program for the device specified
	status = clBuildProgram(*program, 1, &dev, NULL, NULL, NULL);

	if(status != CL_SUCCESS)
	{
		cl_build_status build_status;
		clGetProgramBuildInfo(*program, 
				              dev, 
							  CL_PROGRAM_BUILD_STATUS, 
							  sizeof(cl_build_status), 
							  &build_status, 
							  NULL);

		if(build_status == CL_SUCCESS) 
		{
			printf("No compilation errors for this device\n");
		}

		size_t ret_val_size;
		clGetProgramBuildInfo(*program, 
				              dev, 
							  CL_PROGRAM_BUILD_LOG, 
							  0, 
							  NULL, 
							  &ret_val_size);

		char* build_log = NULL;
		build_log = (char *)malloc(ret_val_size+1);
		if(build_log == NULL)
		{
			perror("malloc");
			exit(1);
		}

		clGetProgramBuildInfo(*program, dev, CL_PROGRAM_BUILD_LOG, ret_val_size+1, build_log, NULL);
		build_log[ret_val_size] = '\0';

		printf("Build log:\n %s...\n", build_log);
	}



	assert(status == CL_SUCCESS);

	assert(*program != NULL);
	return 0;
}

/*
 * \brief Create kernel.
 */
int CreateKernel(cl_kernel *kernel, cl_context ctx, cl_program program)
{
	cl_int status = 0;

	kernel[0] = clCreateKernel(program, "MatrixMult_naive", &status);
	assert(status == CL_SUCCESS);

	kernel[1] = clCreateKernel(program, "MatrixMult_opt", &status);
	assert(status == CL_SUCCESS);

	return 0;
}

/*
 * \brief Enqueue a kernel run call. Wait till it completes.
 */
int RunKernel(cl_command_queue cmd_q, cl_kernel kernel, int hA, int wB, const char *info)
{
	printf("\nStart executing %s\n", info);
	int hC = hA;
	int wC = wB;

	const size_t local_size[2]  = {16, 16};
	const size_t global_size[2] = {(wC + 15) / 16 * 16, (hC + 15) / 16 * 16};

	cl_int status = clEnqueueNDRangeKernel(
			cmd_q,
			kernel,
			2,
			NULL,
			global_size,
			local_size,
			0,
			NULL,
			NULL);
	assert(status == CL_SUCCESS);

	// Wait for the kernel call to finish execution
	status = clFinish(cmd_q);
	assert(status == CL_SUCCESS);

	printf("Finish executing %s\n", info);

	return 0;
}


/*
 * \brief Check results from device.
 */
int Check(float *c_host, float *c_ref, int hC, int wC)
{
	bool passed = true;

	int i, j;
	for (i = 0; i < hC; ++i)
	{
		for (j = 0; j < wC; ++j)
		{
			if (fabs(c_host[i * wC + j] - c_ref[i * wC + j]) > 1e-5)
			{
				passed = false;
				break;
			}
		}
	}

	if (passed)
	{
		fprintf(stderr, "Passed!\n");
	}
	else
	{
		fprintf(stderr, "Failed!\n");
	}

	return 0;
}

/*
 * \brief Compute Matrix Multiplication on CPU.
 */
void cpu_mm(float *a, float *b, float *c, int hA, int wA, int wB)
{
	// write your code here

}

int main(int argc, char *argv[])
{
	printf("Start Program.\n");
	cl_int status;

	// fixed dim: 320 x 320
	int wA = 320;
	int hA = 320;
	size_t bytes_a = wA * hA * sizeof(float);
	float *a_host = (float *) malloc(bytes_a);
	assert(a_host);

	// fixed dim: 320 x 640
	int wB = 640;
	int hB = wA;
	size_t bytes_b = wB * hB * sizeof(float);
	float *b_host = (float *) malloc(bytes_b);
	assert(b_host);

	// 320 x 640
	int wC = wB;
	int hC = hA;
	size_t bytes_c = wC * hC * sizeof(float);
	float *c_host_k0 = (float *) malloc(bytes_c);
	assert(c_host_k0);

	float *c_host_k1 = (float *) malloc(bytes_c);
	assert(c_host_k1);

	// Initialize matrix a and b
	int i;
	for (i = 0; i < wA * hA; ++i)
	{
		a_host[i] = 0.1f;
	}

	for (i = 0; i < wB * hB; ++i)
	{
		b_host[i] = 1.f;
	}

	float *c_ref = (float *) malloc(bytes_c);
	assert(c_ref);

	// Compute on CPU
	printf("\nComputing Matrix Multiplication on CPU\n");
	cpu_mm(a_host, b_host, c_ref, hA, wA, wB);
	printf("Done.\n");


	////////////////////////////////////////////////////////////////////
	// STEP 1 Get platform
	////////////////////////////////////////////////////////////////////
	cl_platform_id platform = NULL;
	GetPlatform(&platform);

	////////////////////////////////////////////////////////////////////
	// STEP 2 Create context using the platform selected
	//        Context created from type includes all available
	//        devices of the specified type from the selected platform
	////////////////////////////////////////////////////////////////////
	cl_context ctx = NULL;
	CreateContext(&ctx, platform);

	////////////////////////////////////////////////////////////////////
	// STEP 3 Get device
	////////////////////////////////////////////////////////////////////
	cl_device_id device = NULL;
	GetDevice(&device, ctx);

	////////////////////////////////////////////////////////////////////
	// STEP 4 Create command queue for a single device
	//        Each device in the context can have a 
	//        dedicated command queue object for itself
	////////////////////////////////////////////////////////////////////
	cl_command_queue cmd_q = NULL;
	CreateCommandQueue(&cmd_q, ctx, device);

	////////////////////////////////////////////////////////////////////
	// STEP 5 Create device buffer and write the inputs to GPU
	//        These buffer objects can be passed to the kernel
	//        as kernel arguments
	////////////////////////////////////////////////////////////////////
	cl_mem a_dev = NULL;
	cl_mem b_dev = NULL;
	cl_mem c_dev_k0 = NULL;
	cl_mem c_dev_k1 = NULL;

	a_dev = clCreateBuffer(
			ctx, 
			CL_MEM_READ_ONLY,
			bytes_a,
			NULL, 
			&status);
	assert(status == CL_SUCCESS);

	b_dev = clCreateBuffer(
			ctx, 
			CL_MEM_READ_ONLY,
			bytes_b,
			NULL, 
			&status);
	assert(status == CL_SUCCESS);

	c_dev_k0 = clCreateBuffer(
			ctx, 
			CL_MEM_READ_WRITE,
			bytes_c,
			NULL, 
			&status);
	assert(status == CL_SUCCESS);

	c_dev_k1 = clCreateBuffer(
			ctx, 
			CL_MEM_READ_WRITE,
			bytes_c,
			NULL, 
			&status);
	assert(status == CL_SUCCESS);
	
	// Transfer data to device
	status = clEnqueueWriteBuffer(
			 cmd_q,
			 a_dev,
			 CL_TRUE,
			 0,
			 bytes_a,
			 a_host,
			 0,
			 NULL,
			 NULL);
	assert(status == CL_SUCCESS);

	status = clEnqueueWriteBuffer(
			 cmd_q,
			 b_dev,
			 CL_TRUE,
			 0,
			 bytes_b,
			 b_host,
			 0,
			 NULL,
			 NULL);
	assert(status == CL_SUCCESS);

	////////////////////////////////////////////////////////////////////
	// STEP 6 Get program and kernel
	////////////////////////////////////////////////////////////////////
	cl_program program = NULL;
	CreateProgram(&program, ctx, device);

	cl_kernel kernel[2];
	CreateKernel(kernel, ctx, program);

	////////////////////////////////////////////////////////////////////
	// STEP 7 Set kernel arguments
	////////////////////////////////////////////////////////////////////
	
	// naive version
	status = clSetKernelArg(kernel[0], 0, sizeof(cl_mem), (void *)&a_dev);
	assert(status == CL_SUCCESS);

	status = clSetKernelArg(kernel[0], 1, sizeof(cl_mem), (void *)&b_dev);
	assert(status == CL_SUCCESS);

	status = clSetKernelArg(kernel[0], 2, sizeof(cl_mem), (void *)&c_dev_k0);
	assert(status == CL_SUCCESS);

	status = clSetKernelArg(kernel[0], 3, sizeof(int), (void *)&wA);
	assert(status == CL_SUCCESS);

	status = clSetKernelArg(kernel[0], 4, sizeof(int), (void *)&wB);
	assert(status == CL_SUCCESS);

	// optimized version
	status = clSetKernelArg(kernel[1], 0, sizeof(cl_mem), (void *)&a_dev);
	assert(status == CL_SUCCESS);

	status = clSetKernelArg(kernel[1], 1, sizeof(cl_mem), (void *)&b_dev);
	assert(status == CL_SUCCESS);

	status = clSetKernelArg(kernel[1], 2, sizeof(cl_mem), (void *)&c_dev_k1);
	assert(status == CL_SUCCESS);

	status = clSetKernelArg(kernel[1], 3, sizeof(int), (void *)&wA);
	assert(status == CL_SUCCESS);

	status = clSetKernelArg(kernel[1], 4, sizeof(int), (void *)&wB);
	assert(status == CL_SUCCESS);

	////////////////////////////////////////////////////////////////////
	// STEP 8 Enqueue a kernel run call
	//        Wait till the kernel completes
	////////////////////////////////////////////////////////////////////
	RunKernel(cmd_q, kernel[0], hA, wB, "naive kernel");


	////////////////////////////////////////////////////////////////////
	// STEP 9 Enqueue a command to read the output back from GPU
	//        Wait till the readback completes
	////////////////////////////////////////////////////////////////////
	status = clEnqueueReadBuffer(
			cmd_q,
			c_dev_k0,
			CL_TRUE,
			0,
			bytes_c,
			c_host_k0,
			0,
			NULL,
			NULL);
	assert(status == CL_SUCCESS);

	// Verify output
	Check(c_host_k0, c_ref, hC, wC);


	RunKernel(cmd_q, kernel[1], hA, wB, "optimized kernel");

	status = clEnqueueReadBuffer(
			cmd_q,
			c_dev_k1,
			CL_TRUE,
			0,
			bytes_c,
			c_host_k1,
			0,
			NULL,
			NULL);
	assert(status == CL_SUCCESS);

	// Verify output
	Check(c_host_k1, c_ref, hC, wC);

	////////////////////////////////////////////////////////////////////
	// STEP 10  Clean up the OpenCL resources
	////////////////////////////////////////////////////////////////////
	status = clReleaseMemObject(c_dev_k0);
	assert(status == CL_SUCCESS);
	status = clReleaseMemObject(c_dev_k1);
	assert(status == CL_SUCCESS);
	status = clReleaseMemObject(b_dev);
	assert(status == CL_SUCCESS);
	status = clReleaseMemObject(a_dev);
	assert(status == CL_SUCCESS);

	for(i=0; i<2; ++i)
	{
		status = clReleaseKernel(kernel[i]);
		assert(status == CL_SUCCESS);
	}
	status = clReleaseProgram(program);
	assert(status == CL_SUCCESS);
	status = clReleaseCommandQueue(cmd_q);
	assert(status == CL_SUCCESS);
	status = clReleaseContext(ctx);
	assert(status == CL_SUCCESS);

	// Release host resources
	free(c_host_k0);
	c_host_k0 = NULL;
	free(c_host_k1);
	c_host_k1 = NULL;
	free(b_host);
	b_host = NULL;
	free(a_host);
	a_host = NULL;
	free(c_ref);
	c_ref = NULL;

	printf("\nEnd of program.\n");

	return 0;
}

