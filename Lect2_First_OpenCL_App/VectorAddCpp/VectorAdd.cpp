#include <CL/cl.hpp>

#include <cassert>
#include <cmath>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>



/*
 * \brief Get platform.
 */
int GetPlatform(cl::Platform &platform)
{
	// Get all the available platforms
	std::vector<cl::Platform> platforms;
	cl_int status = cl::Platform::get(&platforms);
	assert(status == CL_SUCCESS);
	assert(!platforms.empty());

	// Choose the AMD platform
	for (auto platforms_iter = platforms.begin(); 
			platforms_iter != platforms.end(); ++platforms_iter)
	{
		std::string vendor;
		status = platforms_iter->getInfo(CL_PLATFORM_VENDOR, &vendor);
		assert(status == CL_SUCCESS);
		if (vendor == "Advanced Micro Devices, Inc.")
		{
			platform = *platforms_iter;
			break;
		}
	}

	std::cerr << __func__ << std::endl;
	return 0;
}

/*
 * \brief Create context.
 */
int CreateContext(cl::Context &ctx, cl::Platform &platform)
{
	// Set up context properties
	cl_context_properties prop[] = {
			CL_CONTEXT_PLATFORM, 
			(cl_context_properties)platform(), 
			0};

	// Create a context with all the available GPUs
	cl_int status = 0;
	ctx = cl::Context(CL_DEVICE_TYPE_GPU, prop, 0, 0, &status);
	assert(status == CL_SUCCESS);

	std::cerr << __func__ << std::endl;
	return 0;
}

/*
 * \brief Get device available in the given context.
 */
int GetDevice(cl::Device &dev, cl::Context &ctx)
{
	// Get the first device
	std::vector<cl::Device> devices;
	cl_int status = ctx.getInfo(CL_CONTEXT_DEVICES, &devices);
	assert(status == CL_SUCCESS);
	assert(!devices.empty());

	dev = cl::Device(devices[0]);

	std::cerr << __func__ << std::endl;
	return 0;
}

/*
 * \brief Create command queue for the given device.
 */
int CreateCommandQueue(cl::CommandQueue &cmd_q, cl::Context &ctx, 
		cl::Device &dev)
{
	cl_int status = 0;
	cmd_q = cl::CommandQueue(ctx, dev, 0, &status);
	assert(status == CL_SUCCESS);

	std::cerr << __func__ << std::endl;
	return 0;
}

/*
 * \brief Create device buffers.
 */
int CreateBuffer(cl::Buffer &a_dev, cl::Buffer &b_dev, cl::Buffer &c_dev, 
		cl::Context &ctx, const size_t size)
{
	cl_int status = 0;
	a_dev = cl::Buffer(ctx, CL_MEM_READ_WRITE, size, 0, &status);
	assert(status == CL_SUCCESS);
	b_dev = cl::Buffer(ctx, CL_MEM_READ_WRITE, size, 0, &status);
	assert(status == CL_SUCCESS);
	c_dev = cl::Buffer(ctx, CL_MEM_READ_WRITE, size, 0, &status);
	assert(status == CL_SUCCESS);

	std::cerr << __func__ << std::endl;
	return 0;
}

/*
 * \brief Enqueue a command to write the input to GPU. Wait till it completes.
 */
int WriteToGPU(cl::CommandQueue &cmd_q, cl::Buffer &buf_dev, float *buf_host, 
		int size)
{
	cl_int status = cmd_q.enqueueWriteBuffer(
			buf_dev,
			CL_TRUE,
			0,
			size,
			buf_host,
			0,
			0);
	assert(status == CL_SUCCESS);

	std::cerr << __func__ << std::endl;
	return 0;
}

/*
 * \brief Create and build program.
 */
int CreateProgram(cl::Program &program, cl::Context &ctx, cl::Device &dev)
{
	// Convert the contents of a file into a string
	std::ifstream kernel_file("VectorAdd_Kernel.cl");
	assert(kernel_file.is_open());
	std::string src(
			(std::istreambuf_iterator<char>(kernel_file)), 
			std::istreambuf_iterator<char>());
	kernel_file.close();

	// Create a program
	cl_int status = 0;
	program = cl::Program(ctx, src, false, &status);
	assert(status == CL_SUCCESS);

	// Build the program
	status = program.build("", 0, 0);
	assert(status == CL_SUCCESS);

	std::cerr << __func__ << std::endl;
	return 0;
}

/*
 * \brief Create kernel.
 */
int CreateKernel(cl::Kernel &kernel, cl::Program &program)
{
	cl_int status = 0;
	kernel = cl::Kernel(program, "VectorAddKernel", &status);
	assert(status == CL_SUCCESS);

	std::cerr << __func__ << std::endl;
	return 0;
}

/*
 * \brief Set kernel arguments.
 */
int SetKernelArg(cl::Kernel &kernel, cl::Buffer &c_dev, cl::Buffer &a_dev, 
		cl::Buffer &b_dev, int num_elem)
{
	cl_int status = 0;
	status = kernel.setArg(0, c_dev);
	assert(status == CL_SUCCESS);
	status = kernel.setArg(1, a_dev);
	assert(status == CL_SUCCESS);
	status = kernel.setArg(2, b_dev);
	assert(status == CL_SUCCESS);
	status = kernel.setArg(3, num_elem);
	assert(status == CL_SUCCESS);

	std::cerr << __func__ << std::endl;
	return 0;
}

/*
 * \brief Enqueue a kernel run call. Wait till it completes.
 */
int RunKernel(cl::CommandQueue &cmd_q, cl::Kernel &kernel, int num_elem)
{
	cl::NDRange offset(0);
	cl::NDRange global((num_elem + 255) / 256 * 256);
	cl::NDRange local(256);

	cl_int status = cmd_q.enqueueNDRangeKernel(
			kernel,
			offset,
			global,
			local,
			0,
			0);
	assert(status == CL_SUCCESS);

	// Wait for the kernel call to finish execution
	status = cmd_q.finish();
	assert(status == CL_SUCCESS);

	std::cerr << __func__ << std::endl;
	return 0;
}

/*
 * \brief Enqueue a command to read the output back. Wait till it completes.
 */
int ReadFromGPU(cl::CommandQueue &cmd_q, cl::Buffer &c_dev, float *c_host, 
		int size)
{
	cl_int status = cmd_q.enqueueReadBuffer(
			c_dev,
			CL_TRUE,
			0,
			size,
			c_host,
			0,
			0);
	assert(status == CL_SUCCESS);

	std::cerr << __func__ << std::endl;
	return 0;
}

/*
 * \brief Print no more than 16 elements of the given array.
 */
int Print1DArray(const char *array_name, const float *array, const int num_elem)
{
	int num_elem_to_print = (num_elem > 16) ? 16 : num_elem;

	std::cerr << array_name << ":" << std::endl;
	for (int i = 0; i < num_elem_to_print; ++i)
	{
		std::cerr << array[i] << " ";
	}
	std::cerr << std::endl;

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
		if (std::abs(a_host[i] + b_host[i] - c_host[i]) > 1e-5)
		{
			passed = false;
			break;
		}
	}

	if (passed)
	{
		std::cerr << "Passed!" << std::endl;
	}
	else
	{
		std::cerr << "Failed!" << std::endl;
	}

	return 0;
}


int main(int argc, char *argv[])
{
	// Allocate and initialize host buffers
	assert(argc == 2);
	const int num_elem = std::atoi(argv[1]);
	assert(num_elem > 0);
	float *a_host = new float[num_elem];
	assert(a_host);
	float *b_host = new float[num_elem];
	assert(b_host);
	float *c_host = new float[num_elem];
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
	cl::Platform platform;
	GetPlatform(platform);

	////////////////////////////////////////////////////////////////////
	// STEP 2 Create context using the platform selected
	//        Context created from type includes all available
	//        devices of the specified type from the selected platform
	////////////////////////////////////////////////////////////////////
	cl::Context ctx;
	CreateContext(ctx, platform);

	////////////////////////////////////////////////////////////////////
	// STEP 3 Get device
	////////////////////////////////////////////////////////////////////
	cl::Device device;
	GetDevice(device, ctx);

	////////////////////////////////////////////////////////////////////
	// STEP 4 Create command queue for a single device
	//        Each device in the context can have a 
	//        dedicated command queue object for itself
	////////////////////////////////////////////////////////////////////
	cl::CommandQueue cmd_q;
	CreateCommandQueue(cmd_q, ctx, device);

	////////////////////////////////////////////////////////////////////
	// STEP 5 Create device buffer and write the inputs to GPU
	//        These buffer objects can be passed to the kernel
	//        as kernel arguments
	////////////////////////////////////////////////////////////////////
	cl::Buffer a_dev;
	cl::Buffer b_dev;
	cl::Buffer c_dev;
	CreateBuffer(a_dev, b_dev, c_dev, ctx, num_elem * sizeof(float));
	WriteToGPU(cmd_q, a_dev, a_host, num_elem * sizeof(float));
	WriteToGPU(cmd_q, b_dev, b_host, num_elem * sizeof(float));

	////////////////////////////////////////////////////////////////////
	// STEP 6 Get program and kernel
	////////////////////////////////////////////////////////////////////
	cl::Program program;
	CreateProgram(program, ctx, device);
	cl::Kernel kernel;
	CreateKernel(kernel, program);

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
	// Do nothing here since we allocate OpenCL resources on stack.
	// Delete as normal C++ if they are on heap.

	// Release host resources
	delete[] c_host;
	delete[] b_host;
	delete[] a_host;

	return 0;
}

