#include <stdio.h>
#include <stdlib.h>
#include <syscall.h>

int main(int argc, char **argv)
{
	int fib=0;
	int sum=0;
	int a,b,c,d;
	int i;

	a=atoi(argv[1]);
	b=atoi(argv[2]);
	c=atoi(argv[3]);
	d=atoi(argv[4]);

	printf("Input : %s",argv[0]);
	
	for(i=0;i<4;i++)
		printf(" %d",atoi(argv[i+1]));
	printf("\n");
	
	fib = fibonacci(a);
	sum = sum_four_int(a,b,c,d);
	printf("%d %d\n", fib, sum);

	return EXIT_SUCCESS;
}
