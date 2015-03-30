#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <CL/cl.h>
#include <math.h>
#include <stdbool.h>
#define CHECK_STATUS( status, message )   \
		if(status != CL_SUCCESS) \
		{ \
			printf( message); \
			printf( "\n" ); \
			return 1; \
		}




/** Define custom constants*/
#define MAX_SOURCE_SIZE (0x100000)

cl_uint numTap = 0;
cl_uint numData = 0;		
cl_uint numTotalData = 0;
cl_uint numBlocks = 0;
cl_float* input = NULL;
cl_float* output = NULL;
cl_float* coeff = NULL;
cl_float* historyInput = NULL;
cl_event event;


float* cpu_compute(float *input, float* coeff, unsigned int numTap, unsigned int numData);

int main(int argc , char** argv) {

	/** Define Custom Variables */
	int i,count;
	int local;

	if (argc < 3)
	{
		printf(" Usage : ./FIR <numTaps> <numData>\n");
		exit(0);
	}
	if (argc > 1)
	{
		numTap = atoi(argv[1]);
		numData = atoi(argv[2]);
	}


	/** Declare the Filter Properties */
	numBlocks = 1; // Reserved for advanced FIR
	numTotalData = numData * numBlocks;
	local = 64;

	printf("FIR Filter\n Data Samples : %d \n NumTaps : %d \n Local Workgroups : %d\n", numData, numTap, local);
	
	/** Define variables here */
	input = (cl_float *) malloc( numTotalData* sizeof(cl_float));
	output = (cl_float *) malloc( numTotalData* sizeof(cl_float) );
	coeff = (cl_float *) malloc( numTap * sizeof(cl_float) );
	historyInput = (cl_float *) malloc( (numData+numTap-1) * sizeof(cl_float) );

	/** Initialize the input data */
	for( i=0; i < numTotalData; i++ )
	{
		input[i] = (i * rand());
		output[i] = i;
	}

	for( i=0; i < numTap; i++ )
	{
		coeff[i] = 1.0f * rand() /numTap;
		historyInput[i] = 0.0f;
	}
	
	// Perform serial CPU computation
	float *cpu_out = cpu_compute(input, coeff, numTap, numData);

	
	/** Input read from data file, for streaming application
	 *
	// Read the input file
	FILE *fip;
	i=0;
	fip = fopen("temp.dat","r");
	while(i<numTotalData)
	{
		fscanf(fip,"%f",&input[i]);
		i++;
	}
	fclose(fip);
	*
	**/

	// Load the kernel source code into the array source_str
	FILE *fp;
	char *source_str;
	size_t source_size;

	fp = fopen("FIR.cl", "r");
	if (!fp) {
		fprintf(stderr, "Failed to load kernel.\n");
		exit(1);
	}
	source_str = (char*)malloc(MAX_SOURCE_SIZE);
	source_size = fread( source_str, 1, MAX_SOURCE_SIZE, fp);
	fclose( fp );

	// Get platform and device information
	cl_platform_id platform_id = NULL;
	cl_device_id device_id = NULL;
	cl_uint ret_num_devices;
	cl_uint ret_num_platforms;
	cl_int ret = clGetPlatformIDs(1, &platform_id, &ret_num_platforms);
	ret = clGetDeviceIDs( platform_id, CL_DEVICE_TYPE_ALL, 1,
			&device_id, &ret_num_devices);

	printf("/n No of Devices %d",ret_num_platforms );

	char *platformVendor;
	size_t platInfoSize;
	clGetPlatformInfo(platform_id, CL_PLATFORM_VENDOR, 0, NULL,
			&platInfoSize);

	platformVendor = (char*)malloc(platInfoSize);

	clGetPlatformInfo(platform_id, CL_PLATFORM_VENDOR, platInfoSize,
			platformVendor, NULL);
	printf("\tVendor: %s\n", platformVendor);
	free(platformVendor);


	// Create an OpenCL context
	cl_context context = clCreateContext( NULL, 1, &device_id, NULL, NULL, &ret);
	CHECK_STATUS( ret,"Error: Create Context\n");

	// Create a command queue
	cl_command_queue command_queue = clCreateCommandQueue(context, device_id, CL_QUEUE_PROFILING_ENABLE, &ret);
	CHECK_STATUS( ret,"Error: Create Command Queue\n");


	// Create memory buffers on the device for each vector
	cl_mem outputBuffer = clCreateBuffer(context, CL_MEM_READ_WRITE,
			sizeof(cl_float) * numData, NULL, &ret);
	CHECK_STATUS( ret,"Error: Create output Buffer\n");
	cl_mem coeffBuffer = clCreateBuffer(context, CL_MEM_READ_ONLY,
			sizeof(cl_float) * numTap, NULL, &ret);
	CHECK_STATUS( ret,"Error: Create coeff buffer Buffer\n");
	cl_mem tempInputBuffer = clCreateBuffer(context, CL_MEM_READ_WRITE,
			sizeof(cl_float) * (numData+numTap-1), NULL, &ret);
	CHECK_STATUS( ret,"Error: Create temp out buffer Buffer\n");

	// Create a program from the kernel source
	cl_program program = clCreateProgramWithSource(context, 1,
			(const char **)&source_str, (const size_t *)&source_size, &ret);

	// Build the program
	ret = clBuildProgram(program, 1, &device_id, NULL, NULL, NULL);
	if(ret != CL_SUCCESS)
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
	CHECK_STATUS( ret,"Error: Build Program\n");

	// Create the OpenCL kernel
	cl_kernel kernel = clCreateKernel(program, "FIR", &ret);
	CHECK_STATUS( ret,"Error: Create kernel. (clCreateKernel)\n");


	// Set the arguments of the kernel
	ret = clSetKernelArg(kernel, 0, sizeof(cl_mem), (void *)&outputBuffer);
	ret = clSetKernelArg(kernel, 1, sizeof(cl_mem), (void *)&coeffBuffer);
	ret = clSetKernelArg(kernel, 2, sizeof(cl_mem), (void *)&tempInputBuffer);
	ret = clSetKernelArg(kernel, 3, sizeof(cl_uint), (void *)&numTap);

	// Fill Coefficient Buffer
	ret = clEnqueueWriteBuffer(command_queue,
			coeffBuffer,
			1,
			0,
			numTap * sizeof(cl_float),
			coeff,
			0,
			0,
			NULL);

	// Fill History Buffer
	ret = clEnqueueWriteBuffer(command_queue,
			tempInputBuffer,
			1,
			0,
			(numTap) *sizeof(cl_float),
			historyInput,
			0,
			0,
			NULL);


	// Decide the local group formation
	size_t globalThreads[1]={numData};
	size_t localThreads[1]={local};
	count = 0;

	/* Fill in the input buffer object */
	ret = clEnqueueWriteBuffer(command_queue,
			tempInputBuffer,
			1,
			(numTap-1)*sizeof(cl_float),
			numData * sizeof(cl_float),
			input + (count * numData),
			0,
			0,
			NULL);

	// Execute the OpenCL kernel on the list
	ret = clEnqueueNDRangeKernel(
			command_queue,
			kernel,
			1,
			NULL,
			globalThreads,
			localThreads,
			0,
			NULL,
			&event);
	
	CHECK_STATUS( ret,"Error: Range kernel. (clCreateKernel)\n");			
	ret = clWaitForEvents(1, &event);

	// Print kernel execution time
	cl_ulong t_start = 0;
	ret = clGetEventProfilingInfo(
			event, 
			CL_PROFILING_COMMAND_START, 
			sizeof(cl_ulong), 
			&t_start, 
			NULL);
	CHECK_STATUS(ret, "Profiling event fail\n");
	
	cl_ulong t_end = 0;
	ret = clGetEventProfilingInfo(
			event, 
			CL_PROFILING_COMMAND_END, 
			sizeof(cl_ulong), 
			&t_end, 
			NULL);
	CHECK_STATUS(ret, "Profiling event fail\n");
	fprintf(stderr, "\tKernel exec time: %8.2f us\n", 
			1.f * (t_end - t_start) / 1e3);

	/* Get the output buffer */
	ret = clEnqueueReadBuffer(
			command_queue,
			outputBuffer,
			CL_TRUE,
			0,
			numData * sizeof( cl_float ),
			output + count * numData,
			0,
			NULL,
			NULL);

	/* Uncomment to print output */
	//printf("\n The Output:\n");
	//i = 0;
	//while( i<numTotalData )
	//{
	//	printf( "%f ", output[i] );

	//	i++;
	//}
	//
	
	bool passed = false;
	for (i = 0; i < numData; i++) 
	{
		if ((output[i]) != cpu_out[i]) {
			passed = false;
			break;
		}
		else
			passed = true;
	}
	if(!passed)
		printf("FIR Fail\n");
	else
		printf("FIR Successful\n");
	


	ret= clFlush(command_queue);
	ret = clFinish(command_queue);
	ret = clReleaseKernel(kernel);
	ret = clReleaseProgram(program);
	ret = clReleaseMemObject(outputBuffer);
	ret = clReleaseMemObject(coeffBuffer);
	ret = clReleaseMemObject(tempInputBuffer);
	ret = clReleaseCommandQueue(command_queue);
	ret = clReleaseContext(context);

	free(input);
	free(output);
	free(coeff);
	free(historyInput);
	free(cpu_out);


	return 0;
}


float* cpu_compute(float *input, float* coeff, unsigned int numTap, unsigned int numData)
{
	float *out_cpu, *temp_in;
	out_cpu = (float*) malloc (numData * sizeof(float));
	temp_in = (float*) calloc ((numData + numTap - 1) , sizeof(float));
	
	memcpy((temp_in + numTap - 1) , input, sizeof(float) * numData);
	float sum;
	for(int i = 0; i < numData; i++)
	{
		sum = 0.f;
		for(int j = 0; j < numTap; j++)
			sum += coeff[j] * temp_in[i + j];
		out_cpu[i] = sum;
	}
	
	free(temp_in);
	return out_cpu;
}















