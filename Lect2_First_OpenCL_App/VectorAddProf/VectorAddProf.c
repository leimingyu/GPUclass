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

	assert(*platform);
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
			CL_DEVICE_TYPE_GPU | CL_DEVICE_TYPE_CPU, 
			NULL, 
			NULL, 
			&status);
	assert(status == CL_SUCCESS);

	assert(*ctx);
	return 0;
}

/*
 * \brief Get GPU and CPU devices in the given context.
 */
int GetDevice(cl_device_id *dev_gpu, cl_device_id *dev_cpu, cl_context ctx)
{
	// Get the number of devices in a given context
	size_t num_dev = 0;
	cl_int status = clGetContextInfo(
			ctx, 
			CL_CONTEXT_DEVICES, 
			0, 
			NULL, 
			&num_dev);
	assert(status == CL_SUCCESS);
	assert(num_dev >= 2);

	// Get all the devices in the context
	cl_device_id *dev = (cl_device_id *) calloc(num_dev, 
			sizeof(cl_device_id));
	assert(dev);
	status = clGetContextInfo(
			ctx, 
			CL_CONTEXT_DEVICES, 
			num_dev * sizeof(cl_device_id), 
			dev, 
			NULL);
	assert(status == CL_SUCCESS);

	// Get the device according to its device type
	for (size_t i = 0; i < num_dev; ++i)
	{
		cl_device_type t = 0;
		status = clGetDeviceInfo(
				dev[i], 
				CL_DEVICE_TYPE, 
				sizeof(cl_device_type), 
				&t, 
				NULL);
		assert(status == CL_SUCCESS);
		if (t == CL_DEVICE_TYPE_GPU)
			*dev_gpu = dev[i];
		else if (t == CL_DEVICE_TYPE_CPU)
			*dev_cpu = dev[i];
		if (*dev_gpu && *dev_cpu)
			break;
	}

	free(dev);
	dev = NULL;

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
			CL_QUEUE_PROFILING_ENABLE, 
			&status);
	assert(status == CL_SUCCESS);

	assert(*cmd_q);
	return 0;
}

/*
 * \brief Create device buffers from host buffers.
 */
int CreateBuffer(cl_mem *a_dev, cl_mem *b_dev, cl_mem *c_dev_gpu, 
		cl_mem *c_dev_cpu, cl_context ctx, void *a_host, void *b_host, 
		void *c_host_gpu, void *c_host_cpu, const size_t size)
{
	cl_int status = 0;
	*a_dev = clCreateBuffer(
			ctx, 
			CL_MEM_READ_WRITE | CL_MEM_USE_HOST_PTR,
			size,
			a_host, 
			&status);
	assert(status == CL_SUCCESS);
	*b_dev = clCreateBuffer(
			ctx, 
			CL_MEM_READ_WRITE | CL_MEM_USE_HOST_PTR,
			size,
			b_host, 
			&status);
	assert(status == CL_SUCCESS);
	*c_dev_gpu = clCreateBuffer(
			ctx, 
			CL_MEM_READ_WRITE | CL_MEM_USE_HOST_PTR,
			size,
			c_host_gpu, 
			&status);
	assert(status == CL_SUCCESS);
	*c_dev_cpu = clCreateBuffer(
			ctx, 
			CL_MEM_READ_WRITE | CL_MEM_USE_HOST_PTR,
			size,
			c_host_cpu, 
			&status);
	assert(status == CL_SUCCESS);

	assert(*a_dev);
	assert(*b_dev);
	assert(*c_dev_gpu);
	assert(*c_dev_cpu);
	return 0;
}

/*
 * \brief Create program.
 */
int CreateProgram(cl_program *program, cl_context ctx)
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

	assert(*program);
	return 0;
}

/*
 * \brief Build program.
 */
int BuildProgram(cl_program program, cl_device_id dev)
{
	cl_int status = clBuildProgram(program, 1, &dev, NULL, NULL, NULL);
	assert(status == CL_SUCCESS);

	return 0;
}

/*
 * \brief Create kernel.
 */
int CreateKernel(cl_kernel *kernel, cl_context ctx, cl_program program)
{
	cl_int status = 0;
	*kernel = clCreateKernel(program, "VectorAddKernel", &status);
	assert(status == CL_SUCCESS);

	assert(*kernel);
	return 0;
}

/*
 * \brief Set kernel arguments.
 */
int SetKernelArg(cl_kernel kernel, cl_mem c_dev, cl_mem a_dev, cl_mem b_dev, 
		int num_elem)
{
	cl_int status = 0;
	status = clSetKernelArg(
			kernel, 
			0, 
			sizeof(cl_mem), 
			(void *)&c_dev);
	assert(status == CL_SUCCESS);
	status = clSetKernelArg(
			kernel, 
			1, 
			sizeof(cl_mem), 
			(void *)&a_dev);
	assert(status == CL_SUCCESS);
	status = clSetKernelArg(
			kernel, 
			2, 
			sizeof(cl_mem), 
			(void *)&b_dev);
	assert(status == CL_SUCCESS);
	status = clSetKernelArg(
			kernel, 
			3, 
			sizeof(int), 
			(void *)&num_elem);
	assert(status == CL_SUCCESS);

	return 0;
}

/*
 * \brief Enqueue a kernel run call. Wait till it completes.
 */
int RunKernel(cl_command_queue cmd_q, cl_kernel kernel, int num_elem)
{
	const size_t global_size[1] = {(num_elem + 255) / 256 * 256};
	const size_t local_size[1] = {256};
	cl_event e;

	cl_int status = clEnqueueNDRangeKernel(
			cmd_q,
			kernel,
			1,
			NULL,
			global_size,
			local_size,
			0,
			NULL,
			&e);
	assert(status == CL_SUCCESS);

	status = clFinish(cmd_q);
	assert(status == CL_SUCCESS);

	// Print kernel execution time
	cl_ulong t_start = 0;
	status = clGetEventProfilingInfo(
			e, 
			CL_PROFILING_COMMAND_START, 
			sizeof(cl_ulong), 
			&t_start, 
			NULL);
	assert(status == CL_SUCCESS);
	cl_ulong t_end = 0;
	status = clGetEventProfilingInfo(
			e, 
			CL_PROFILING_COMMAND_END, 
			sizeof(cl_ulong), 
			&t_end, 
			NULL);
	assert(status == CL_SUCCESS);
	fprintf(stderr, "\tKernel exec time: %8.2f us\n", 
			1.f * (t_end - t_start) / 1e3);

	return 0;
}

/*
 * \brief Enqueue a command to read the output back. Wait till it completes.
 */
int ReadFromGPU(cl_command_queue cmd_q, cl_mem c_dev, float *c_host, 
		int size)
{
	cl_int status = clEnqueueReadBuffer(
			cmd_q,
			c_dev,
			CL_TRUE,
			0,
			size,
			c_host,
			0,
			NULL,
			NULL);
	assert(status == CL_SUCCESS);

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
		fprintf(stderr, "%8.2f ", array[i]);
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

	if (!passed)
		fprintf(stderr, "Failed!\n");

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
	float *c_host_gpu = (float *) calloc(num_elem, sizeof(float));
	assert(c_host_gpu);
	float *c_host_cpu = (float *) calloc(num_elem, sizeof(float));
	assert(c_host_cpu);

	for (int i = 0; i < num_elem; ++i)
	{
		a_host[i] = i * 0.9f;
		b_host[i] = i * 1.1f;
	}

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
	cl_device_id dev_gpu = NULL;
	cl_device_id dev_cpu = NULL;
	GetDevice(&dev_gpu, &dev_cpu, ctx);

	////////////////////////////////////////////////////////////////////
	// STEP 4 Create command queue for a single device
	//        Each device in the context can have a 
	//        dedicated command queue object for itself
	////////////////////////////////////////////////////////////////////
	cl_command_queue cq_gpu = NULL;
	CreateCommandQueue(&cq_gpu, ctx, dev_gpu);
	cl_command_queue cq_cpu = NULL;
	CreateCommandQueue(&cq_cpu, ctx, dev_cpu);

	////////////////////////////////////////////////////////////////////
	// STEP 5 Create device buffer from host buffer
	//        These buffer objects can be passed to the kernel
	//        as kernel arguments
	////////////////////////////////////////////////////////////////////
	cl_mem a_dev = NULL;
	cl_mem b_dev = NULL;
	cl_mem c_dev_gpu = NULL;
	cl_mem c_dev_cpu = NULL;
	CreateBuffer(&a_dev, &b_dev, &c_dev_gpu, &c_dev_cpu, ctx, a_host, 
			b_host, c_host_gpu, c_host_cpu, 
			num_elem * sizeof(float));

	////////////////////////////////////////////////////////////////////
	// STEP 6 Get kernel
	////////////////////////////////////////////////////////////////////
	cl_program prog_gpu = NULL;
	CreateProgram(&prog_gpu, ctx);
	BuildProgram(prog_gpu, dev_gpu);
	cl_program prog_cpu = NULL;
	CreateProgram(&prog_cpu, ctx);
	BuildProgram(prog_cpu, dev_cpu);
	cl_kernel kern_gpu = NULL;
	CreateKernel(&kern_gpu, ctx, prog_gpu);
	cl_kernel kern_cpu = NULL;
	CreateKernel(&kern_cpu, ctx, prog_cpu);

	////////////////////////////////////////////////////////////////////
	// STEP 7 Set kernel arguments
	////////////////////////////////////////////////////////////////////
	SetKernelArg(kern_gpu, c_dev_gpu, a_dev, b_dev, num_elem);
	SetKernelArg(kern_cpu, c_dev_cpu, a_dev, b_dev, num_elem);

	////////////////////////////////////////////////////////////////////
	// STEP 8 Enqueue a kernel run call
	//        Wait till the kernel completes
	////////////////////////////////////////////////////////////////////
	fprintf(stderr, "GPU\n");
	RunKernel(cq_gpu, kern_gpu, num_elem);
	fprintf(stderr, "CPU\n");
	RunKernel(cq_cpu, kern_cpu, num_elem);

	////////////////////////////////////////////////////////////////////
	// STEP 9 Enqueue a command to read the output back
	//        Wait till the readback completes
	////////////////////////////////////////////////////////////////////
	ReadFromGPU(cq_gpu, c_dev_gpu, c_host_gpu, num_elem * sizeof(float));
	ReadFromGPU(cq_cpu, c_dev_cpu, c_host_cpu, num_elem * sizeof(float));

	// Verify output
	Verify(a_host, b_host, c_host_gpu, num_elem);
	Verify(a_host, b_host, c_host_cpu, num_elem);

	////////////////////////////////////////////////////////////////////
	// STEP 10  Clean up the OpenCL resources
	////////////////////////////////////////////////////////////////////
	status = clReleaseKernel(kern_cpu);
	assert(status == CL_SUCCESS);
	status = clReleaseKernel(kern_gpu);
	assert(status == CL_SUCCESS);
	status = clReleaseProgram(prog_cpu);
	assert(status == CL_SUCCESS);
	status = clReleaseProgram(prog_gpu);
	assert(status == CL_SUCCESS);
	status = clReleaseMemObject(c_dev_cpu);
	assert(status == CL_SUCCESS);
	status = clReleaseMemObject(c_dev_gpu);
	assert(status == CL_SUCCESS);
	status = clReleaseMemObject(b_dev);
	assert(status == CL_SUCCESS);
	status = clReleaseMemObject(a_dev);
	assert(status == CL_SUCCESS);
	status = clReleaseCommandQueue(cq_cpu);
	assert(status == CL_SUCCESS);
	status = clReleaseCommandQueue(cq_gpu);
	assert(status == CL_SUCCESS);
	status = clReleaseContext(ctx);
	assert(status == CL_SUCCESS);

	// Release host resources
	free(c_host_cpu);
	c_host_cpu = NULL;
	free(c_host_gpu);
	c_host_gpu = NULL;
	free(b_host);
	b_host = NULL;
	free(a_host);
	a_host = NULL;

	return 0;
}

