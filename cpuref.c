#include <stdint.h>
#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <string.h>

#include "timing.c"
#include "dump_buf.h"

static void print_32x4s(char *name, unsigned int a, unsigned int b, unsigned int c, unsigned int d)
{
	printf("%s: 0x%08x 0x%08x 0x%08x 0x%08x\n", name, a, b, c, d);
}


static inline void xor_salsa8(unsigned int B[16], const unsigned int Bx[16])
{
	unsigned int x00,x01,x02,x03,x04,x05,x06,x07,x08,x09,x10,x11,x12,x13,x14,x15;
	int i;

	x00 = (B[ 0] ^= Bx[ 0]);
	x01 = (B[ 1] ^= Bx[ 1]);
	x02 = (B[ 2] ^= Bx[ 2]);
	x03 = (B[ 3] ^= Bx[ 3]);
	x04 = (B[ 4] ^= Bx[ 4]);
	x05 = (B[ 5] ^= Bx[ 5]);
	x06 = (B[ 6] ^= Bx[ 6]);
	x07 = (B[ 7] ^= Bx[ 7]);
	x08 = (B[ 8] ^= Bx[ 8]);
	x09 = (B[ 9] ^= Bx[ 9]);
	x10 = (B[10] ^= Bx[10]);
	x11 = (B[11] ^= Bx[11]);
	x12 = (B[12] ^= Bx[12]);
	x13 = (B[13] ^= Bx[13]);
	x14 = (B[14] ^= Bx[14]);
	x15 = (B[15] ^= Bx[15]);
	for (i = 0; i < 8; i += 2) {
#define R(a, b) (((a) << (b)) | ((a) >> (32 - (b))))
		/* Operate on columns. */
		x04 ^= R(x00+x12, 7);	x09 ^= R(x05+x01, 7);
		x14 ^= R(x10+x06, 7);	x03 ^= R(x15+x11, 7);
		
		x08 ^= R(x04+x00, 9);	x13 ^= R(x09+x05, 9);
		x02 ^= R(x14+x10, 9);	x07 ^= R(x03+x15, 9);
		
		x12 ^= R(x08+x04,13);	x01 ^= R(x13+x09,13);
		x06 ^= R(x02+x14,13);	x11 ^= R(x07+x03,13);
		
		x00 ^= R(x12+x08,18);	x05 ^= R(x01+x13,18);
		x10 ^= R(x06+x02,18);	x15 ^= R(x11+x07,18);
		
		/* Operate on rows. */
		x01 ^= R(x00+x03, 7);	x06 ^= R(x05+x04, 7);
		x11 ^= R(x10+x09, 7);	x12 ^= R(x15+x14, 7);
		
		x02 ^= R(x01+x00, 9);	x07 ^= R(x06+x05, 9);
		x08 ^= R(x11+x10, 9);	x13 ^= R(x12+x15, 9);
		
		x03 ^= R(x02+x01,13);	x04 ^= R(x07+x06,13);
		x09 ^= R(x08+x11,13);	x14 ^= R(x13+x12,13);
		
		x00 ^= R(x03+x02,18);	x05 ^= R(x04+x07,18);
		x10 ^= R(x09+x08,18);	x15 ^= R(x14+x13,18);
#undef R
	}
	B[ 0] += x00;
	B[ 1] += x01;
	B[ 2] += x02;
	B[ 3] += x03;
	B[ 4] += x04;
	B[ 5] += x05;
	B[ 6] += x06;
	B[ 7] += x07;
	B[ 8] += x08;
	B[ 9] += x09;
	B[10] += x10;
	B[11] += x11;
	B[12] += x12;
	B[13] += x13;
	B[14] += x14;
	B[15] += x15;
}


static void salsa8_test(void)
{
	int i;
	unsigned int B[16], Bx[16];

	for (i = 0; i < 16; i++) {
		B[i] = (i + 1) * 2;
		Bx[i] = (i + 1) * 4;
	}

	printf("IN\n");
	print_32x4s(" B[ 0]", B[0], B[1], B[2], B[3]);
	print_32x4s(" B[ 4]", B[4], B[5], B[6], B[7]);
	print_32x4s(" B[ 8]", B[8], B[9], B[10], B[11]);
	print_32x4s(" B[12]", B[12], B[13], B[14], B[15]);
	print_32x4s("Bx[ 0]", Bx[0], Bx[1], Bx[2], Bx[3]);
	print_32x4s("Bx[ 4]", Bx[4], Bx[5], Bx[6], Bx[7]);
	print_32x4s("Bx[ 8]", Bx[8], Bx[9], Bx[10], Bx[11]);
	print_32x4s("Bx[12]", Bx[12], Bx[13], Bx[14], Bx[15]);

	xor_salsa8(B, Bx);
	printf("\n");

	printf("OUT\n");
	print_32x4s(" B[ 0]", B[0], B[1], B[2], B[3]);
	print_32x4s(" B[ 4]", B[4], B[5], B[6], B[7]);
	print_32x4s(" B[ 8]", B[8], B[9], B[10], B[11]);
	print_32x4s(" B[12]", B[12], B[13], B[14], B[15]);
}


static inline void scrypt_core(uint32_t *X, uint32_t *V, int N)
{
	int i;
	struct timespec loop1_start, loop1_end, loop2_end;

	get_time(&loop1_start);
	for (i = 0; i < N; i++) {
		memcpy(&V[i * 32], X, 128);
		xor_salsa8(&X[0], &X[16]);
		xor_salsa8(&X[16], &X[0]);
	}

	get_time(&loop1_end);

	for (i = 0; i < N; i++) {
		uint32_t j = 32 * (X[16] & (N - 1));
		for (uint8_t k = 0; k < 32; k++)
			X[k] ^= V[j + k];
		xor_salsa8(&X[0], &X[16]);
		xor_salsa8(&X[16], &X[0]);
	}
	get_time(&loop2_end);

	printf("loop1: %f s, loop2: %f s\n", time_diff(&loop1_start, &loop1_end), time_diff(&loop1_end, &loop2_end));

}

static void scrypt_test(void)
{
	struct timespec start, end;
	int N = 1048576;
	size_t size = 32 * (N + 1) * sizeof(uint32_t);
	printf("size=%u\n", size);
	unsigned char *scratchpad;

	posix_memalign((void **)&scratchpad, 4096, size);

	uint8_t X[32*4] = {
		0x2c, 0xea, 0xa9, 0x0e, 0x59, 0x44, 0x8a, 0x45, 0x31, 0x89, 0x2e, 0xac, 0xf5, 0xb8, 0x7b, 0x22,
		0x63, 0xfe, 0xb1, 0xf2, 0x78, 0xca, 0xf4, 0x65, 0x0a, 0xe8, 0x3e, 0xc1, 0xb9, 0xa8, 0xd6, 0x9d,
		0x62, 0x09, 0xa7, 0x37, 0x6e, 0x55, 0x24, 0xce, 0xaf, 0x81, 0x90, 0x16, 0x4c, 0x6c, 0xa0, 0x73,
		0xbe, 0xfb, 0xef, 0x7f, 0x14, 0x86, 0x18, 0x90, 0x52, 0x41, 0x9f, 0x49, 0xcf, 0x00, 0x4f, 0x17,
		0xa9, 0x89, 0x2f, 0x5a, 0x71, 0xd1, 0x98, 0x9f, 0x82, 0x07, 0xf5, 0x2f, 0xb1, 0x51, 0xc5, 0xc8,
		0xa2, 0xfb, 0x4a, 0xcf, 0xf0, 0x45, 0x97, 0x08, 0x1f, 0x3b, 0x55, 0x37, 0xec, 0x0e, 0xa6, 0xbc,
		0x25, 0xd2, 0x3e, 0x19, 0xa1, 0x2d, 0x4c, 0x0d, 0x74, 0x06, 0x67, 0x4a, 0x5c, 0x64, 0x20, 0x44,
		0x7e, 0xad, 0x2e, 0x43, 0x96, 0x84, 0x0b, 0xa7, 0x34, 0x23, 0x99, 0x1d, 0xde, 0x14, 0x2b, 0x84,
	};

	uint8_t X_out[32*4] = {
		0x0c, 0xfe, 0x4d, 0xa2, 0x90, 0xbc, 0x02, 0xe1, 0x9a, 0x43, 0x1d, 0x78, 0x5e, 0x27, 0xa7, 0x0a,
		0xb4, 0x24, 0x73, 0xd8, 0x54, 0x9a, 0x71, 0xb4, 0xd2, 0x80, 0xc4, 0x78, 0xd4, 0x61, 0x66, 0xf7,
		0xaf, 0x58, 0xbb, 0xa2, 0x63, 0x05, 0xfc, 0x6f, 0x37, 0x90, 0x64, 0x6f, 0x86, 0x94, 0xcd, 0x8b,
		0x8d, 0xbc, 0x22, 0xfc, 0xcd, 0x60, 0x94, 0x52, 0xf8, 0x63, 0x8a, 0x17, 0xbd, 0x14, 0x0a, 0x5b,
		0x32, 0x64, 0x69, 0x74, 0xb6, 0x97, 0x8c, 0x3f, 0x27, 0xcc, 0x96, 0xc4, 0x4e, 0xe2, 0x72, 0x2c,
		0xc8, 0x3f, 0x71, 0xc4, 0x27, 0xb0, 0xcc, 0xf4, 0xe2, 0xf6, 0x3e, 0x04, 0x7c, 0x13, 0x2a, 0xb2,
		0x86, 0x4d, 0x58, 0xde, 0xb9, 0x1a, 0x0d, 0xb6, 0xc1, 0x94, 0x08, 0xc7, 0x6a, 0xe1, 0xc4, 0x2e,
		0x67, 0x1c, 0xdb, 0x5c, 0xf9, 0xcf, 0x8f, 0xe8, 0xd1, 0x94, 0x06, 0xbe, 0xb9, 0x54, 0x2f, 0x05,
	};

	get_time(&start);
	scrypt_core((void *)X, (void *)scratchpad, N);
	get_time(&end);

	dump_buf("X out", X, 32 * 4);

	printf("elapsed: %f s\n", time_diff(&start, &end));
}

int main()
{
	scrypt_test();
	return 0;
}
