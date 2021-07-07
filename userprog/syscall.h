#ifndef USERPROG_SYSCALL_H
#define USERPROG_SYSCALL_H
//#define OPEN 1
//#define CLOSE 0
void syscall_init (void);

/* prj2_1 */
void exit (int);
void halt(void);
int write(int, const void*, unsigned);
int read(int, void *,unsigned);
int exec(const char *);
int wait(int);
int fibonacci(int );
int sum_four_int(int a,int b,int c, int d);


typedef struct myfile { 
	struct file* list;
	struct myfile* next;
	int fd;
	int ch;
	char name[50];
	bool status;
} myfile;
struct myfile *myfilelist, head;

struct myfile* findfile(int fd);
int thread_num;
#endif /* userprog/syscall.h */
