#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <errno.h>
#include <sys/stat.h>

#include <CL/cl.h>
#include <CL/cl_ext.h>

#include "clutils.c"

void rot13(char *buf)
{
	int index = 0;
	char c = buf[index];
	while (c != 0) {
		if (c < 'A' || c > 'z' || (c > 'Z' && c < 'a')) {
			buf[index] = buf[index];
		} else {
			if (c > 'm' || (c > 'M' && c < 'a')) {
				buf[index] = buf[index] - 13;
			} else {
				buf[index] = buf[index] + 13;
			}
		}
		c = buf[++index];
	}
}




int main(void)
{
	char *buf = NULL, *buf2 = NULL;
	size_t worksize, srcsize, pwet;

	cl_int error;
	cl_platform_id platform;
	cl_device_id device;
	cl_uint platforms, devices;

	/* Fetch the Platform and Device IDs; we only want one. */
	error = clGetPlatformIDs(1, &platform, &platforms);
	error = clGetDeviceIDs(platform, CL_DEVICE_TYPE_ALL, 1, &device, &devices);
	cl_context_properties properties[] = {
		CL_CONTEXT_PLATFORM, (cl_context_properties)platform,
		CL_PRINTF_CALLBACK_ARM, (cl_context_properties)printf_callback,
		CL_PRINTF_BUFFERSIZE_ARM, 0x1000,
		0
	};
	// Note that nVidia's OpenCL requires the platform property
	cl_context context = clCreateContext(properties, 1, &device, NULL, NULL, &error);
	cl_command_queue cq = clCreateCommandQueue(context, device, 0, &error);

	cl_kernel k_rot13 = load_kernel(context, "./rot13.cl", "rot13");

	worksize = strlen("hello, World!");

	/* Allocate memory for the kernel to work with */
	cl_mem mem1, mem2;
	mem1 = clCreateBuffer(context, CL_MEM_READ_ONLY | CL_MEM_ALLOC_HOST_PTR, 128 * 1024 * 1024, NULL, &error);
	buf = clEnqueueMapBuffer(cq, mem1, CL_TRUE, CL_MAP_READ, 0, 128 * 1024 * 1024, 0, NULL, NULL, &error);

	mem2 = clCreateBuffer(context, CL_MEM_WRITE_ONLY | CL_MEM_ALLOC_HOST_PTR, 128 * 1024 * 1024, NULL, &error);
	buf2 = clEnqueueMapBuffer(cq, mem2, CL_TRUE, CL_MAP_WRITE, 0, 128 * 1024 * 1024, 0, NULL, NULL, &error);

	strcpy(buf, "Hello, World!");

	rot13(buf);		// scramble using the CPU
	puts(buf);		// Just to demonstrate the plaintext is destroyed

	memset(buf2, 0, 128);
	buf2[0] = '?';


	/* get a handle and map parameters for the kernel */
	//cl_kernel k_rot13 = clCreateKernel(prog, "rot13", &error);
	clSetKernelArg(k_rot13, 0, sizeof(mem1), &mem1);
	clSetKernelArg(k_rot13, 1, sizeof(mem2), &mem2);

	/* Send input data to OpenCL (async, don't alter the buffer!) */
	error = clEnqueueWriteBuffer(cq, mem1, CL_FALSE, 0, worksize, buf, 0, NULL, NULL);

	/* Perform the operation */
	error = clEnqueueNDRangeKernel(cq, k_rot13, 1, NULL, &nthreads, &nthreads, 0, NULL, NULL);
	printf("enqueue task error=%d\n", error);

	/* Read the result back into buf2 */
	error = clEnqueueReadBuffer(cq, mem2, CL_FALSE, 0, worksize, buf2, 0, NULL, NULL);
	
	/* Await completion of all the above */
	error = clFinish(cq);
	printf("finish error: %d\n", error);

	/* Finally, output out happy message. */
	puts(buf2);

	return 0;
}
