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
	const char *filename  = "kernel.cl";
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

	kernel[0] = clCreateKernel(program, "matrix_transpose", &status);
	assert(status == CL_SUCCESS);

	return 0;
}

/*
 * \brief Enqueue a kernel run call. Wait till it completes.
 */
int RunKernel(cl_command_queue cmd_q, cl_kernel kernel, int hA, int wA, const char *info)
{
	printf("\nStart executing %s\n", info);

	// customize your tile size here
	// Input Matrix [hA][wA]
	// Output Matrix [wA][hA]
	// dim x : hA (col)	
	// dim y : WA (row)	
	const size_t local_size[2]  = {16, 16};  
	const size_t global_size[2] = {(hA + 15) / 16 * 16, (wA + 15) / 16 * 16};

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
 * \brief Compute Matrix Transpose on CPU.
 */
void cpu_transpose(float *a, float *aT, int hA, int wA)
{
	// dim for a : hA x wA
	// dim for aT : wA x hA
	int row, col;
	for(row = 0; row < hA; row++)
	{
		for(col = 0; col < wA; col++) 
		{
			// aT[col][row] = a[row][col];
			aT[col * hA + row] = a[row * wA + col];
		}
	}
}

/*
 * \brief Check results from device.
 */
int Check(float *a_host, float *a_ref, int hA, int wA)
{
	bool passed = true;

	int i, j;
	for (i = 0; i < hA; ++i)
	{
		for (j = 0; j < wA; ++j)
		{
			if (fabs(a_host[i * wA + j] - a_ref[i * wA + j]) > 1e-5)
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


//----------------------------------------------------------------------------//
//	Main Function
//----------------------------------------------------------------------------//
int main(int argc, char *argv[])
{
	printf("Start Program.\n");
	cl_int status;

	//------------------------------------------------------------------------//
	// Input Matrix
	//------------------------------------------------------------------------//
	int wA = 320;
	int hA = 320;
	size_t bytes_a = wA * hA * sizeof(float);
	float *a_host = (float *) malloc(bytes_a);
	assert(a_host);

	float *aT_host = (float *) malloc(bytes_a); // transposed output
	assert(aT_host);


	// Initialize matrix a and b
	int i;
	for (i = 0; i < wA * hA; ++i)
	{
		a_host[i] = (float) i;
	}

	// CPU transposed results
	float *aT_ref = (float *) malloc(bytes_a);
	assert(aT_ref);

	// Compute on CPU
	printf("\nComputing Matrix Transpose on CPU\n");
	cpu_transpose(a_host, aT_ref, hA, wA);
	printf("Done.\n");


	//------------------------------------------------------------------------//
	// STEP 1 Get platform
	//------------------------------------------------------------------------//
	cl_platform_id platform = NULL;
	GetPlatform(&platform);

	//------------------------------------------------------------------------//
	// STEP 2 Create context using the platform selected
	//        Context created from type includes all available
	//        devices of the specified type from the selected platform
	//------------------------------------------------------------------------//
	cl_context ctx = NULL;
	CreateContext(&ctx, platform);

	//------------------------------------------------------------------------//
	// STEP 3 Get device
	//------------------------------------------------------------------------//
	cl_device_id device = NULL;
	GetDevice(&device, ctx);

	//------------------------------------------------------------------------//
	// STEP 4 Create command queue for a single device
	//        Each device in the context can have a 
	//        dedicated command queue object for itself
	//------------------------------------------------------------------------//
	cl_command_queue cmd_q = NULL;
	CreateCommandQueue(&cmd_q, ctx, device);

	//------------------------------------------------------------------------//
	// STEP 5 Create device buffer and write the inputs to GPU
	//        These buffer objects can be passed to the kernel
	//        as kernel arguments
	//------------------------------------------------------------------------//
	cl_mem a_dev = NULL;
	cl_mem aT_dev = NULL;

	a_dev = clCreateBuffer(
			ctx, 
			CL_MEM_READ_ONLY,
			bytes_a,
			NULL, 
			&status);
	assert(status == CL_SUCCESS);

	aT_dev = clCreateBuffer(
			ctx, 
			CL_MEM_READ_WRITE,
			bytes_a,
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

	////////////////////////////////////////////////////////////////////
	//------------------------------------------------------------------------//
	// STEP 6 Get program and kernel
	//------------------------------------------------------------------------//
	////////////////////////////////////////////////////////////////////
	cl_program program = NULL;
	CreateProgram(&program, ctx, device);

	cl_kernel kernel[1];
	CreateKernel(kernel, ctx, program);

	//------------------------------------------------------------------------//
	// STEP 7 Set kernel arguments
	//------------------------------------------------------------------------//
	status = clSetKernelArg(kernel[0], 0, sizeof(cl_mem), (void *)&a_dev);
	assert(status == CL_SUCCESS);

	status = clSetKernelArg(kernel[0], 1, sizeof(cl_mem), (void *)&aT_dev);
	assert(status == CL_SUCCESS);

	status = clSetKernelArg(kernel[0], 2, sizeof(int), (void *)&hA);
	assert(status == CL_SUCCESS);

	status = clSetKernelArg(kernel[0], 3, sizeof(int), (void *)&wA);
	assert(status == CL_SUCCESS);


	//------------------------------------------------------------------------//
	// STEP 8 Enqueue a kernel run call
	//        Wait till the kernel completes
	//------------------------------------------------------------------------//
	RunKernel(cmd_q, kernel[0], hA, wA, "matrix transpose kernel");


	//------------------------------------------------------------------------//
	// STEP 9 Enqueue a command to read the output back from GPU
	//        Wait till the readback completes
	//------------------------------------------------------------------------//
	status = clEnqueueReadBuffer(
			cmd_q,
			aT_dev,
			CL_TRUE,
			0,
			bytes_a,
			aT_host,
			0,
			NULL,
			NULL);
	assert(status == CL_SUCCESS);

	// Verify output
	Check(aT_host, aT_ref, wA, hA);


	//------------------------------------------------------------------------//
	// STEP 10  Clean up the OpenCL resources
	//------------------------------------------------------------------------//
	status = clReleaseMemObject(a_dev);
	assert(status == CL_SUCCESS);
	status = clReleaseMemObject(aT_dev);
	assert(status == CL_SUCCESS);

	for(i=0; i<1; ++i)
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
	free(a_host);
	free(aT_host);
	free(aT_ref);

	a_host  = NULL;
	aT_host = NULL;
	aT_ref  = NULL;

	printf("\nEnd of program.\n");

	return 0;
}

