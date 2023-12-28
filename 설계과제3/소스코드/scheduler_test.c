#include "types.h"
#include "stat.h"
#include "user.h"

#ifndef PNUM
	#define PNUM 3
#endif

void scheduler_func(void)
{
	int pid,i = 0;

	printf(1,"start scheduler_test\n");
	
	for(i = 0; i < PNUM; i++)
	{
		if((pid = fork()) < 0)
			exit();
		else if(pid == 0)
		{
			if((i % 3) == 0)
				set_sche_info(1, 110);
			else if((i % 3) == 1)
				set_sche_info(22, 200);
			else if((i % 3) == 2)
				set_sche_info(11, 250);

			while(1);
		}
	}

	for(i = 0; i < PNUM; i++)
		wait();

}

int main(void)
{
	scheduler_func();
	printf(2,"end of scheduler test\n");
	exit();
}
