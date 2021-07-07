#include "process.h"
#include "devices/input.h"
#include "userprog/syscall.h"
#include <stdio.h>
#include <string.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "filesys/filesys.h"
#include "filesys/file.h"
#include "filesys/off_t.h"

#include "lib/user/syscall.h"
#include "devices/shutdown.h"
#include "threads/vaddr.h"
#include "threads/malloc.h"


static void syscall_handler (struct intr_frame *);
int fd_num;
int file_num;
int debugging=0;


struct myfile* findfile(int fd)
{
	struct myfile *temp = myfilelist;

	while(temp) {
		if(temp->fd == fd)
			return temp;
		temp = temp->next;
	}

	return NULL;
}

void syscall_init (void) 
{
	intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
}

void exit(int status){
	int i=0;
	int par_exit;
	struct thread* cur_thr; 
	char thread_name[50];

	cur_thr = thread_current();
	sema_down(&(cur_thr->parent->child_lock[cur_thr->tid]));
	if(debugging == 1){
		printf("[exit] entered exit function. thread: %s\n",cur_thr->name);
	}
	strlcpy(thread_name, cur_thr->name, strlen(cur_thr->name)+1);
	while(1){
		if(thread_name[i]==' ' || thread_name[i]=='\0'){
			thread_name[i] = '\0';
			break;
		}
		i++;
	}
	if(status >=0)
		par_exit = status;
	else{
		par_exit = -1;
	}
	cur_thr->parent->exit_status = par_exit;

	if(debugging == 1){
		printf("[exit] after informing parent thread about status of current thread : %d\n", 
				cur_thr->parent->exit_status);
	}
	printf("%s: exit(%d)\n",thread_name,cur_thr->parent->exit_status);

	cur_thr->parent->child_cnt--;

	//here!
	sema_up(&(cur_thr->mem_lock));
	thread_initial();
	file_allow_write(cur_thr->file);
	thread_print();
	thread_exit();
}


void halt(void) {

	shutdown_power_off();

}


int read(int fd, void *buffer, unsigned size){
	unsigned i=0;
	struct file* temp;

	if(buffer == NULL) return -1;
	if(buffer + size > PHYS_BASE) exit(-1);

	if(fd != 0){
		if(debugging == 1){
			printf(" read 1!! (%s)=0\n",(char*)buffer);
		}
		temp = findfile(fd)->list; 
		i= (unsigned)file_read(temp,buffer,size);
		return (uint32_t)i;
	}
	else{
		while(i<size){
			*(char*)(buffer+(i)) = input_getc();

			i++;
		}
		return (uint32_t)size;
	}
}

int write(int fd, const void *buffer, unsigned size){
	struct file* temp;
	if(buffer == NULL) return -1;
	if(buffer + size > PHYS_BASE) exit(-1);

	if(fd!=1){
		temp = findfile(fd)->list;
		off_t i= file_write(temp, buffer, size);
		return (uint32_t)i;
	}
	else{
		putbuf(buffer, size);
		return (uint32_t)size;
	}
}

int exec (const char *cmd_line) {
	return process_execute(cmd_line);
}


int wait(int pid) {

	return process_wait(pid);

}


int sum_four_int(int a,int b,int c,int d){

	return a+b+c+d;

}

int fibonacci(int a) {

	int x=1,y=1;
	int i=3;
	int z;
	if( a==1 || a==2 )
		return 1;
	else{
		while(i <= a){
			z = x+y;
			x = y;
			y = z;
			i++;
		}
	}
	return y;

}


bool create(const char* file,unsigned initial_size)
{
	if(file==NULL || file + initial_size -1 >= PHYS_BASE )
		exit(-1);

	//sema_down(&read_lock);
	bool ret = filesys_create(file, initial_size);	

	return ret;

}

bool remove(const char *file){

	if(!file)
		exit(-1);

	sema_down(&read_lock);
	bool ret = filesys_remove(file);
	sema_up(&read_lock);

	return ret;

}

int open(const char* file)
{
	if(file==NULL)
		exit(-1);
	struct thread* cur = thread_current();
	unsigned int result;
	struct file* temp;
	struct myfile* newf;


	if(cur==NULL||!strcmp("",file)||!strcmp(file,"no-such-file"))
		return -1;

	if(file_num==0)
	{
		//file initialize
		myfilelist = &head;
		myfilelist->next = NULL;
		myfilelist->fd = -1;
		fd_num=2;
		myfilelist->ch=1;
	}

	sema_down(&read_lock);
	temp = filesys_open(file);
	sema_up(&read_lock);

	if(!temp) result = -1;
	else {

		int i=0;
		struct myfile* fp;
		fp = myfilelist;
		while(fp->next){
			fp = fp->next;
		}
		if(debugging==1)
			printf("open(%s)\n",cur->name);
		newf = (myfile*)malloc(sizeof(myfile));

		fp->next = newf;
		fp = fp->next;
		fp->next = NULL;
		if(debugging == 1)
			printf("newf linked\n");

		fp->status = 1;
		fp->fd = fd_num++;

		strlcpy(fp->name, file, strlen(file));
		while(file[i]!='\0') i++;
		fp->name[i]='\0';

		fp->list = temp;
		file_num++;
		result = fp->fd;
	}
	return result;

}


int filesize(int fd)
{
	struct myfile* t;
	struct file *res = NULL;

	for(t=myfilelist; t!=NULL && t->fd != fd ; t= t->next) ;
	if( t->fd == fd){
		res = t->list;
		return file_length(res);
	}

	if(res==NULL){
		return -1;
	}
}

void seek(int fd, unsigned position){
	struct file *temp = findfile(fd)->list;
	file_seek(temp,(off_t)position);
}

unsigned tell(int fd){
	struct file* temp;

	temp = findfile(fd)->list;
	return (unsigned)file_tell(temp);

}
void close(int fd){

	struct file* temp = findfile(fd)->list;
	struct myfile* curmyfile = findfile(fd);
	if(!temp)
		exit(-1);
	if(!(curmyfile->status==1)){
		file_close(temp);
		curmyfile->status = 0;
	}
}


	static void
syscall_handler (struct intr_frame *f UNUSED)
{

	int syscall_num;
	syscall_num = *(int*)f->esp;

	//////////////////////////////// seohee /////////////////////////////////////////////
	if(debugging == 1){
		printf("[syscall handler] current address : %u\n", (uint32_t) (f->esp));
	}

	if (f->esp == NULL || ((uint32_t)(f->esp) + (uint32_t)4) >= (uint32_t)0xc0000000)
		exit(-1);

	int *tmp = (int*)(f->esp);

	if(syscall_num == SYS_HALT)
		halt();
	else if(syscall_num == SYS_EXIT){
		exit(tmp[1]);
	}
	else{
		if(syscall_num==SYS_READ || syscall_num==SYS_WRITE){
			//parameter what?
			int fd = tmp[1];
			void *buff = (void*)tmp[2];
			uint32_t size = (uint32_t)tmp[3];
			switch(syscall_num){
				case SYS_READ:
					f->eax = read(fd, buff, size);
					break;
				case SYS_WRITE:
					f->eax = write(fd,buff,size);
					break;
			}
		}
		else{
			switch(syscall_num){
				//else
				case SYS_EXEC:
					f->eax = exec((char*)tmp[1]); break;
				case SYS_WAIT: 
					f->eax = wait(tmp[1]); break;
					//additional implementation functions.
				case SYS_SUM: 
					f->eax = sum_four_int(tmp[1], tmp[2], tmp[3], tmp[4]); break;
				case SYS_FIBO: 
					f->eax = fibonacci(tmp[1]); break;

					// prj2_2
				case SYS_CREATE :
					f->eax = create((void*)tmp[1], (unsigned)tmp[2]); break;
				case SYS_OPEN :
					//printf("systemcall handler의 open 케이스\n");
					f->eax = open((void*)tmp[1]); break;
				case SYS_REMOVE : f->eax = remove((void*)tmp[1]);break;
				case SYS_FILESIZE:
								  f->eax = filesize(tmp[1]);break;
				case SYS_CLOSE : 
								  close(tmp[1]);break;
				case SYS_SEEK : 
								  seek(tmp[1],tmp[2]);break;
				case SYS_TELL : 
								  f->eax = tell(tmp[1]);break;
				default : break;
			}
		}
	}
	/////////////////////////////////////////////////////////////////////////////////////
}
