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


static void print_B(char *name, unsigned int *B)
{
	printf("%s[ 0]: 0x%08x 0x%08x 0x%08x 0x%08x\n", name, B[0], B[1], B[2], B[3]);
	printf("%s[ 4]: 0x%08x 0x%08x 0x%08x 0x%08x\n", name, B[4], B[5], B[6], B[7]);
	printf("%s[ 8]: 0x%08x 0x%08x 0x%08x 0x%08x\n", name, B[8], B[9], B[10], B[11]);
	printf("%s[12]: 0x%08x 0x%08x 0x%08x 0x%08x\n", name, B[12], B[13], B[14], B[15]);
}



static int nthreads = 32;

typedef struct {
	cl_command_queue cq;
	size_t worksize;
	cl_mem B;
} pwet;

static void event_cb(cl_event ev, cl_int status, unsigned int *B)
{
	int j;

	printf("event_cb: %p %d %p\n", ev, status, B);

	for (j = 0; j < nthreads; j++) {
		print_B(" B", &(B[j * 16]));
		printf("-----\n");
	}
}

int main(void)
{
	size_t worksize;
	cl_context ctx;
	cl_command_queue cq;
	cl_kernel xor_salsa8;
	cl_int error;
	cl_mem B_mem, Bx_mem;
	unsigned int *B = NULL, *Bx = NULL;
	int i, j;
	cl_ulong l;
	size_t wg;

	cl_event ev1, ev2;

	cl_init(&ctx, &cq);

	worksize = 16 * sizeof(unsigned int) * nthreads;

	Bx_mem = clCreateBuffer(ctx, CL_MEM_READ_WRITE | CL_MEM_ALLOC_HOST_PTR, worksize, NULL, &error);
	Bx = clEnqueueMapBuffer(cq, Bx_mem, CL_TRUE, CL_MAP_READ | CL_MAP_WRITE, 0, worksize, 0, NULL, NULL, &error);

	B_mem = clCreateBuffer(ctx, CL_MEM_READ_WRITE | CL_MEM_ALLOC_HOST_PTR, worksize, NULL, &error);
	B = clEnqueueMapBuffer(cq, B_mem, CL_TRUE, CL_MAP_READ | CL_MAP_WRITE, 0, worksize, 0, NULL, NULL, &error);


	xor_salsa8 = load_kernel(ctx, "./salsa8.cl", "xor_salsa8");
	clSetKernelArg(xor_salsa8, 0, sizeof(B_mem), &B_mem);
	clSetKernelArg(xor_salsa8, 1, sizeof(Bx_mem), &Bx_mem);
	clSetKernelArg(xor_salsa8, 2, sizeof(nthreads), &nthreads);

//	printf("IN\n");
	for (j = 0; j < nthreads; j++) {
		int idx = j * 16;
		for (i = 0; i < 16; i++) {
			B[(j * 16) + i] = (i + 1) * (2 + 0);
			Bx[(j * 16) + i] = (i + 1) * (4 + 0);

		}
	}

	error = clEnqueueUnmapMemObject(cq, Bx_mem, Bx, 0, NULL, NULL);
	error = clEnqueueUnmapMemObject(cq, B_mem, B, 0, NULL, NULL);

	/* Perform the operation */
	wg = (size_t)nthreads;
	error = clEnqueueNDRangeKernel(cq, xor_salsa8, 1, NULL, &wg, &wg, 0, NULL, &ev1);
	printf("enqueue task: %d\n", error);

	B = clEnqueueMapBuffer(cq, B_mem, CL_FALSE, CL_MAP_READ, 0, worksize, 1, &ev1, &ev2, NULL);

	printf("set callback: %d\n", clSetEventCallback(ev2, CL_COMPLETE, (void *)event_cb, (void *)B));

	/* Await completion of all the above */
	error = clFlush(cq);
	printf("finish error: %d\n", error);
	clFinish(cq);

	cl_ulong queued = 0, submit = 0, start = 0, end = 0;
	clGetEventProfilingInfo(ev1, CL_PROFILING_COMMAND_QUEUED, sizeof(cl_ulong), &queued, NULL);
	clGetEventProfilingInfo(ev1, CL_PROFILING_COMMAND_SUBMIT, sizeof(cl_ulong), &submit, NULL);
	clGetEventProfilingInfo(ev1, CL_PROFILING_COMMAND_START, sizeof(cl_ulong), &start, NULL);
	clGetEventProfilingInfo(ev1, CL_PROFILING_COMMAND_END, sizeof(cl_ulong), &end, NULL);

	printf("queue: %f ms\n", (submit - queued) / 1000000.0);
	printf("wait: %f ms\n", (start - submit) / 1000000.0);
	printf("run: %f ms\n", (end - start) / 1000000.0);

	return 0;
}


/*scrypt:1048576: ef9dc4d2 1dc07315 4996e0fb debc4c01 4d6defa4 a3791ab1 4bc5def6 8b0d76dc*/

