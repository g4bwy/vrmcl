#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <errno.h>
#include <sys/stat.h>

#include <CL/cl.h>


static void printf_callback(const char *buf, size_t len, size_t final, void *data)
{
	fwrite(buf, 1, len, stdout);
}

static cl_int cl_init(cl_context *ctx, cl_command_queue *cq)
{
	cl_int error;
	cl_platform_id platform;
	cl_device_id device;
	cl_uint platforms, devices;

	if (!ctx || !cq)
		return -1;

	/* Fetch the Platform and Device IDs; we only want one. */
	error = clGetPlatformIDs(1, &platform, &platforms);
	error = clGetDeviceIDs(platform, CL_DEVICE_TYPE_ALL, 1, &device, &devices);
	cl_context_properties properties[] = {
		CL_CONTEXT_PLATFORM, (cl_context_properties)platform,
		CL_PRINTF_CALLBACK_ARM, (cl_context_properties)printf_callback,
		CL_PRINTF_BUFFERSIZE_ARM, 0x1000,
		0
	};
	*ctx = clCreateContext(properties, 1, &device, NULL, NULL, &error);
	*cq = clCreateCommandQueue(*ctx, device, CL_QUEUE_PROFILING_ENABLE, &error);

	return error;
}


static cl_kernel load_kernel(cl_context ctx, char *file, char *name)
{
	int fd = open(file, O_RDONLY);
	const char *buf;
	struct stat st;
	size_t sz;
	cl_program prog;
	cl_kernel ret;
	cl_int err;

	if (fd < 0) {
		fprintf(stderr, "can't load kernel file %s: %s\n", file, strerror(errno));
		return NULL;
	}

	memset(&st, 0, sizeof(st));
	fstat(fd, &st);
	buf = mmap(NULL, st.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
	if (buf == MAP_FAILED) {
		fprintf(stderr, "can't map kernel file %s: %s\n", file, strerror(errno));
		close(fd);
		return NULL;
	}
	close(fd);

	sz = st.st_size;

	prog = clCreateProgramWithSource(ctx, 1, &buf, &sz, &err);
	if (err != CL_SUCCESS) {
		fprintf(stderr, "clCreateProgramWithSource error: %d\n", err);
		munmap((void *)buf, st.st_size);
		return NULL;
	}
	munmap((void *)buf, st.st_size);

	err = clBuildProgram(prog, 0, NULL, "-fkernel-unroller -fkernel-vectorizer", NULL, NULL);
	if (err != CL_SUCCESS) {
		fprintf(stderr, "clBuildProgram error: %d\n", err);
		clReleaseProgram(prog);
		return NULL;
	}

	ret = clCreateKernel(prog, name, &err);
	if (err != CL_SUCCESS) {
		fprintf(stderr, "clCreateKernel error: %d\n", err);
		clReleaseProgram(prog);
		return NULL;
	}

	return ret;
}

