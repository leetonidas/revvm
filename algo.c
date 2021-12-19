#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// 31: hxp{But_whY_w0u1d_you_Do_7h4t?}
// hxp{Wh4t_4_dum6_D3s1gn!1}

#define ai(i,j) (j * 5 + i)
#define swap_row(m,i,j) do				\
{ 							\
	uint8_t tmp[5]; 				\
	memcpy(tmp, (m) + 5 * (i), 5); 			\
	memcpy((m) + 5 * (i), (m) + 5 * (j), 5); 	\
	memcpy((m) + 5 * (j), tmp, 5); 			\
} while(0)

// (det(m)^{-1} * m)^{-1}
const uint8_t mat[25] = {64,84,18,4,91,115,118,92,101,75,53,96,92,25,24,33,34,115,78,33,1,25,99,20,81};

//char flag[25] = "hxp{Wh4t_4_dum6_D3s1gn!1}";
uint8_t flag[30];

uint8_t inm[25] = {0};

uint8_t mul_127(uint8_t a, uint8_t b) {
	uint16_t r = (uint16_t) a * b;
	return r % 127;
}

uint8_t add_127(uint8_t a, uint8_t b) {
	return (a + b) % 127;
}

uint8_t sub_127(uint8_t a, uint8_t b) {
	return a >= b ? a - b : 127 + a - b;
}

/*
typedef struct {
	uint8_t gcd;
	uint8_t ma;
	uint8_t mb;
} egcd_res;

egcd_res inv_egcd(uint8_t a, uint8_t b) {
	if (a == 0){
		egcd_res r = {b, 0, 1};
		//printf("%3d =   0 *   0 + %1$3d => {%1$3d,   0,   1}\n", b);
		return r;
	}
	uint8_t d = b / a;
	uint8_t r = b % a;
	egcd_res res = inv_egcd(r, a);
	uint8_t tmp = res.mb;
	res.mb = res.ma;
	res.ma = res.ma * d + tmp;
	//printf("%3d = %3d * %3d + %3d => {%3d, %3d, %3d}\n", b, d, a, r, res.gcd, res.ma, res.mb);
	return res;
}

uint8_t inv_127(uint8_t a) {
	uint8_t tmp = inv_egcd(a, 127).ma;
	return mul_127(tmp, a) == 1 ? tmp : 127 - tmp;
	return inv_egcd(a, 127).ma;
}*/

uint8_t inv_127(uint8_t a) {
	uint8_t stack[16];
	uint8_t top = 0;
	uint8_t b = 127;
	for (top = 0; a != 1 && b != 1; ++top) {
		if (top & 1) {
			stack[top] = a / b;
			a %= b;
		} else {
			stack[top] = b / a;
			b %= a;
		}
//		printf("%hhd\n", stack[top]);
	}
//	printf("%d steps\n", top);
	a = stack[--top];
//	printf("%hhd\n", a);
	b = 1;
	for (int i = top - 1; i >= 0; --i) {
		uint8_t tmp = b;
//		printf("%hhd %hhd %hhd\n", stack[i], a, b);
		b = a;
		a = stack[i] * a + tmp;	
	}
	return top & 1 ? a : 127 - a;
}

void print_mat(const uint8_t *mat) {
	for (int j = 0; j < 5; ++j) {
		printf("[");
		for (int i = 0; i < 5; ++i) {
			printf("%3d%s", mat[ai(i,j)], i == 4 ? "" : " ");
		}
		printf("]\n");
	}
}

uint8_t det(const uint8_t *m) {
	uint8_t *mc = calloc(5 * 5, sizeof(uint8_t));
	memcpy(mc, m, 5 * 5 * sizeof(uint8_t));
	//print_mat(m);
	uint8_t d = 1;
	for (int i = 0; i < 4; ++i) {
		for (int j = i + 1; j < 5; ++j) {
			//printf("a_{(%d,%d)}\n",i,j);
			if (mc[ai(i,i)] == 0) {
				printf("sawpping rows %d, %d\n", i, j);
				swap_row(mc, i, j);
				d = d == 1 ? 126 : 1;
			} else {
				uint8_t inv = inv_127(mc[ai(i,i)]);
				uint8_t t = mul_127(inv,mc[ai(i,j)]);
				//printf("%3d^{-1} * %3d = %3d * %2$3d = %3d\n", mc[ai(i,i)], mc[ai(i,j)], inv, t);
				//printf("row %d - %d * row %d\n", j, t, i);
				for (int k = i; k < 5; ++k) {
					mc[ai(k,j)] = sub_127(mc[ai(k,j)], mul_127(t, mc[ai(k,i)]));
				}
			}
			//print_mat(mc);
		}
	}
	for (int i = 0; i < 5; ++i)
		d = mul_127(d, mc[ai(i,i)]);
	return d;
}

void mul_mat(uint8_t *r, const uint8_t *a, const uint8_t *b) {
	memset(r, 0, sizeof(uint8_t) * 5 * 5);
	for (int i = 0; i < 5; ++i) {
		for (int j = 0; j < 5; ++j) {
			for (int k = 0; k < 5; ++k)
				r[ai(i,j)] = add_127(r[ai(i,j)], mul_127(a[ai(k,j)], b[ai(i,k)]));
		}
	}
}

int main() {
	printf("flag: ");
	fgets((char *)flag, sizeof(flag), stdin);
/*	for (uint8_t i = 1; i < 127; ++i) {
		printf("inv %hhd: %hhd\n", i, inv_127(i));
	}*/
	printf("determinante: %d\n", det(flag));
	uint8_t rm[25];
	mul_mat(rm, flag, mat);
	printf("\n---------flag--------\n");
	print_mat(flag);
	printf("\n--------matrix-------\n");
	print_mat(mat);
	printf("\n--------result-------\n");
	print_mat(rm);
}
