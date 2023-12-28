#include "types.h"
#include "stat.h"
#include "user.h"

#define BUFFER_SIZE 2048 + 256 // 버퍼 크기 
#define NAME_SIZE   256 // 파일 네임 크기

int line; // 역순으로 출력할 라인 수 전역 변수
char buf[BUFFER_SIZE]; // 버퍼 
char fileName[NAME_SIZE]; // void htac(int fd) 에서 tempFd 파일 디스크립터에 오픈하기 위해 전역 변수 저장

void htac(int fd)
{
	int n,i;
	int tempFd;
	int fileLine = 0;

	// 파일 라인수를 체크
	while((n = read(fd, buf, sizeof(buf))) > 0)
	{
		for(i = 0; i < n; i++)
		{
			if(buf[i] == '\n') fileLine++;
		}
	}
	if(n < 0)
	{
		printf(1, "htac : read error\n");
		exit();
	}

	// tempFd에 역순으로 출력할 파일을 오픈
	if((tempFd = open(fileName, 0)) < 0)
	{
		printf(1, "htac : canoot 2nd open %s\n",fileName);
		exit();
	}

	int index = 0; // 현재 파일 오프셋 
	char buf2[1]; // 파일 오프셋을 옮기기 위해 1문자씩 읽기 위한 버퍼2
	
	// 출력하지 않을 줄은 read를 통해 파일 오프셋을 옮긴다. 
	// line 과  fileLine 같을때는 파일 오프셋을 옮기지 않는다.(전체 출력)
	if(line != fileLine)
	{
		while((n = read(tempFd, buf2, 1)) > 0)
		{
			if(buf2[0] == '\n') index++;
	
			if(index == (fileLine - line)) break;
		}
	}

	// 버퍼 ""으로 값 복사 및 역순으로 출력할 문자열들을 read 한다.
	strcpy(buf, "");
	n = read(tempFd, buf,sizeof(buf));
	
	if(n < 0)
	{
		printf(1, "htac : read error\n");
		exit();
	}

	// start가 '\n'가 될때마다 혹은 start가 음수가 되면  해당 라인을 출력한다.
	// start+1 ~ end-1 까지 출력 후 개행 문자 출력 
	int start = n-2,end = n-1;
	while(start >= 0)
	{
		// 출력할 문자열의 처음을 가리키기 위해 start--
		// 파일에 맨 처음으로 가면 -1(start)  start+1 == 0(파일의 처음)
		// 파일 중간에 '\n'을 만난다. 출력할 문자열의 처음은 start+1
		while(start >= 0 && buf[start] != '\n') 
			start--;

		for(i = start+1; i < end; i++)
		{
			buf2[0] = buf[i];
			if(write(1,buf2,1) != 1)
			{
				printf(1, "htac : write error\n");
				exit();
			}
		}
		printf(1, "\n"); // 개행 문자 출력
		
		// 출력을 완료하고, end를 start로 옮겨줍니다.
		end = start;
		// 다음 문자열 출력을 위해 start인덱스를 -1 합니다.
		start--;
	}

	close(tempFd);
}

int main(int argc, char *argv[])
{
	int fd,i;

	if(argc < 3) 
	{
		// Usage와 동일한 사용을 하지 않은 경우 인자가 3개 미만이라면 Usage 출력
		printf(1, "Usage : %s line file_name\n",argv[0]);
		exit();
	}

	// 역순으로 출력하기 위한 line수 인자를 int형으로 계산한다. atoi 사용대신 구현
	line = 0;
	for(i = 0; i < strlen(argv[1]); i++)
	{
		line *= 10;
		line += argv[1][i] - '0';
	}
	
	// htac에 인자로 전달할 fd를 open()을 통해 저장한다.
	if((fd = open(argv[2], 0)) < 0)
	{
		printf(1,"htac : catnnot 1st open %s\n",argv[2]); // 파일 오픈 에러 처리 
		exit();
	}
	
	strcpy(fileName, argv[2]);
	htac(fd);
	close(fd);

	exit();
}
