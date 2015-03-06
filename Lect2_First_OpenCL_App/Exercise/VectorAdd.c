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
	cl_int status = 0;

	// Get the number of platforms
	cl_uint num_platform = 0;

	// Get all the available platforms
	cl_platform_id *platforms = NULL;

	// Choose the AMD platform
	for (cl_uint i = 0; i < num_platform; ++i)
	{
		char buf[100];
		status = clGetPlatformInfo(
				platforms[i],
				CL_PLATFORM_VENDOR,
				sizeof buf,
				buf,
				NULL);
		assert(status == CL_SUCCESS);
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

	assert(*ctx != NULL);
	return 0;
}

/*
 * \brief Get device available in the given context.
 */
int GetDevice(cl_device_id *dev, cl_context ctx)
{
	// Get the number of devices in a given context

	// Get the first device

	assert(*dev != NULL);
	return 0;
}

/*
 * \brief Create command queue for the given device.
 */
int CreateCommandQueue(cl_command_queue *cmd_q, cl_context ctx, 
		cl_device_id dev)
{

	assert(*cmd_q != NULL);
	return 0;
}

/*
 * \brief Create device buffers.
 */
int CreateBuffer(cl_mem *a_dev, cl_mem *b_dev, cl_mem *c_dev, cl_context ctx, 
		const size_t size)
{

	assert(*a_dev != NULL);
	assert(*b_dev != NULL);
	assert(*c_dev != NULL);
	return 0;
}

/*
 * \brief Enqueue a command to write the input to GPU. Wait till it completes.
 */
int WriteToGPU(cl_command_queue cmd_q, cl_mem buf_dev, float *buf_host, 
		int size)
{

	return 0;
}

/*
 * \brief Create and build program.
 */
int CreateProgram(cl_program *program, cl_context ctx, cl_device_id dev)
{
	// Convert the contents of a file into a string
	const char *filename  = "VectorAdd_Kernel.cl";
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

	// Build the program for the device specified

	assert(*program != NULL);
	return 0;
}

/*
 * \brief Create kernel.
 */
int CreateKernel(cl_kernel *kernel, cl_context ctx, cl_program program)
{

	assert(*kernel != NULL);
	return 0;
}

/*
 * \brief Set kernel arguments.
 */
int SetKernelArg(cl_kernel kernel, cl_mem c_dev, cl_mem a_dev, cl_mem b_dev, 
		int num_elem)
{

	return 0;
}

/*
 * \brief Enqueue a kernel run call. Wait till it completes.
 */
int RunKernel(cl_command_queue cmd_q, cl_kernel kernel, int num_elem)
{
	const size_t global_size[1] = {(num_elem + 255) / 256 * 256};
	const size_t local_size[1] = {256};

	// Enqueue the kernel call

	// Wait for the kernel call to finish execution

	return 0;
}

/*
 * \brief Enqueue a command to read the output back. Wait till it completes.
 */
int ReadFromGPU(cl_command_queue cmd_q, cl_mem c_dev, float *c_host, 
		int size)
{

	return 0;
}

/*
 * \brief Print no more than 16 elements of the given array.
 */
int Print1DArray(const char *array_name, const float *array, const int num_elem)
{
	int num_elem_to_print = (num_elem > 16) ? 16 : num_elem;

	fprintf(stderr, "%s:\n", array_name);
	for (int i = 0; i < num_elem_to_print; ++i)
	{
		fprintf(stderr, "%8.2f ", array[i]);
	}
	fprintf(stderr, "\n");

	return 0;
}

/*
 * \brief Verify results from device.
 */
int Verify(float *a_host, float *b_host, float *c_host, int num_elem)
{
	bool passed = true;
	for (int i = 0; i < num_elem; ++i)
	{
		if (fabs(a_host[i] + b_host[i] - c_host[i]) > 1e-5)
		{
			passed = false;
			break;
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


int main(int argc, char *argv[])
{
	cl_int status;

	// Allocate and initialize host buffers
	assert(argc == 2);
	const int num_elem = atoi(argv[1]);
	assert(num_elem > 0);
	float *a_host = (float *) calloc(num_elem, sizeof(float));
	assert(a_host);
	float *b_host = (float *) calloc(num_elem, sizeof(float));
	assert(b_host);
	float *c_host = (float *) calloc(num_elem, sizeof(float));
	assert(c_host);

	for (int i = 0; i < num_elem; ++i)
	{
		a_host[i] = i;
		b_host[i] = i * 2;
	}

	Print1DArray("A", a_host, num_elem);
	Print1DArray("B", b_host, num_elem);

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
	cl_mem c_dev = NULL;
	CreateBuffer(&a_dev, &b_dev, &c_dev, ctx, num_elem * sizeof(float));
	WriteToGPU(cmd_q, a_dev, a_host, num_elem * sizeof(float));
	WriteToGPU(cmd_q, b_dev, b_host, num_elem * sizeof(float));

	////////////////////////////////////////////////////////////////////
	// STEP 6 Get program and kernel
	////////////////////////////////////////////////////////////////////
	cl_program program = NULL;
	CreateProgram(&program, ctx, device);
	cl_kernel kernel = NULL;
	CreateKernel(&kernel, ctx, program);

	////////////////////////////////////////////////////////////////////
	// STEP 7 Set kernel arguments
	////////////////////////////////////////////////////////////////////
	SetKernelArg(kernel, c_dev, a_dev, b_dev, num_elem);

	////////////////////////////////////////////////////////////////////
	// STEP 8 Enqueue a kernel run call
	//        Wait till the kernel completes
	////////////////////////////////////////////////////////////////////
	RunKernel(cmd_q, kernel, num_elem);

	////////////////////////////////////////////////////////////////////
	// STEP 9 Enqueue a command to read the output back from GPU
	//        Wait till the readback completes
	////////////////////////////////////////////////////////////////////
	ReadFromGPU(cmd_q, c_dev, c_host, num_elem * sizeof(float));

	// Print output array
	Print1DArray("C", c_host, num_elem);

	// Verify output
	Verify(a_host, b_host, c_host, num_elem);

	////////////////////////////////////////////////////////////////////
	// STEP 10  Clean up the OpenCL resources
	////////////////////////////////////////////////////////////////////
	status = clReleaseKernel(kernel);
	assert(status == CL_SUCCESS);
	status = clReleaseProgram(program);
	assert(status == CL_SUCCESS);
	status = clReleaseMemObject(c_dev);
	assert(status == CL_SUCCESS);
	status = clReleaseMemObject(b_dev);
	assert(status == CL_SUCCESS);
	status = clReleaseMemObject(a_dev);
	assert(status == CL_SUCCESS);
	status = clReleaseCommandQueue(cmd_q);
	assert(status == CL_SUCCESS);
	status = clReleaseContext(ctx);
	assert(status == CL_SUCCESS);

	// Release host resources
	free(c_host);
	c_host = NULL;
	free(b_host);
	b_host = NULL;
	free(a_host);
	a_host = NULL;

	return 0;
}

