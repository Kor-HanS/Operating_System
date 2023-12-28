### Operating_System
### 숭실대학교 운영체제 설계 과제 1~4번

원본 소스코드 주소 : https://github.com/mit-pdos/xv6-public

체감 난이도 (쉬움 / 보통 / 어려움)

### 과제 1(쉬움) : XV6 설치 및 간단한 유저프로그램 실행
- XV6 설치를 위해 우분투 상에서 QEMU 에뮬레이터를 통해 소스코드 받아서 돌렸다.

- 진짜 그냥 설치하고, 유저 프로그램 하나 만들면 끝나는 과제 (우리는 htac 이라고 기존 cat 명령어를 반대로, 파일을 끝 ~ 시작 까지 출력하는 프로그램 만들고 Makefile에 넣으면 끝)

### 과제 2(쉬움 ~ 보통) : XV6 시스템 콜 추가 (alarm)
- 기존 코드 분석 요구됨(시스템 콜 호출 과정과 trap 인터럽트 처리 과정등...)

- 시스템 콜 추가를 위한 파일들의 수정 리스트들을 이해해야했다.

- 시스템 콜 호출 방법 및 내부 시스템콜 호출 과정들을 이해해야했다.

- timer interrupt와 관련된 trap() 함수와 이를 구현하기 위한 프로세스의 상태또한 이해해야했다.

- 과제 2에서 부터는 여러 파일들을 만지고, Makefile도 마구 건드리므로, 실수해서 제출 잘못하고 나중에 불상사 생기는 일이 없도록 하자.
  누군가 이 글을 본다면 꼭 과제를 할때마다 원본 XV6 소스코드를 새로 받아서 그 위에 구현하기 바란다. 과제 1, 2 섞여서 낭패본 친구들 많았음.(ㅋㅋ 나도 0점 받고 교수님이 살려주심)
  제발 1번 과제 하고 이어서 하지말고 새로 하자. Makefile에 기존에 과제 한것들 없으면 make 에러 남...

### 과제 3(어려움) : XV6 새로운 스케쥴러 교체 및 기존 XV6 RoundRobin(RR) 스케쥴러와 성능 비교
- 기존 코드 분석 요구됨(과제 2에서의 시스템 콜 + 운영체제 스케쥴러 호출과정 및 스케쥴러 함수 + 프로세스들의 상태 등...)

- 중간 시험 기간 끝나고 7일 동안 빡 코딩 하고 기존 소스코드 분석하고 보고서 쓰고 새로 구현하고 보고서 쓰고 체감상 제일 힘들었다. 
- 리시프 와 운영체제를 들을 우리 후배님들을 위해 오늘도 수고한다고 말해주고 싶다 

- XV6 상에서 커널 프로세스가 호출하는 함수들의 콜 그래프를 이해해야했다. (main() -> mpmain() -> scheduler() 등 커널 프로세스가 다음 cpu가 실행할 프로세스 선정과 관련된 것들)

- 기존 XV6 스케쥴러가 어떤 스케쥴러이며, 어떤 장단점이 있는지 알아야했다. (기존 코드 분석 및 보고서 작성)

- 기존 스케쥴러와 다른 우선순위 큐를 통한 우선순위 스케쥴러 구현을 위한 큐와 관련된 유틸리티 함수들과 구조체들을 새로 만들어야했다. (매우 귀찮았다. == 구현량 증가)
  운영체제 과제들이 기존 XV6 소스코드 위에서 놀아서 그렇지 구현 자체가 어려운건 없다. (리시프같은거 들어서 최소한 2 ~ 3천줄의 구현 프로그램을 짜보길 추천 그거 해봣으면 쉬움)

- 당연히, 스케쥴러가 다음 실행될 프로세스를 고르고 READY 상태내의 프로세스중에 , 실행하고 RUNNING 상태, 실행을 끝내 ZOMBIE 상태가 되고 I/O 상태 BLOCKED(SLEEPING) 등등... 프로세스들의 상태 이해가 필요했다.

- 필요한 것은 운영체제와 같은 큰 프로그램에서 스케쥴러(다음 실행할 프로세스 선정)이라는 놈을 잘 분석하고 갈아끼는 것이다. 분석 잘하고, 새로운 스케쥴러로 갈아끼우고, Makefile 상에서 옵션에 따라 XV6 Default(기존) 스케쥴러 / 새로 구현한 스케쥴러로 번갈아 실행 시켜 분석 하면 끝 


### 과제 4(보통 ~ 어려움) : 파일 시스템 
- 기존 코드 분석 요구됨(기존 파일 시스템 및 inode 구조체 가상 페이지 물리페이지 , trap중에 page fault -> 애는 없어서 구현시켜야했음 , 디스크 블록 할당 / 할당 해제 등...)

- 기존 파일시스템에서 사용했던 inode 구조체를 이해해야했다.

- 1,2,3을 하면서 이미 XV6 상에 코드들을 조금 이해하고 운영체제 지식이 있다면 더 필요한 코드 분석은 2,3 번에 비해 적은 편이었다.

- 2가지 시스템콜을 추가적으로 구현했다. (가상 페이지 수 / 물리 페이지 수 )

- pagefault() 와 같은 기존에 없던 인터럽트 구현이 필요했다. (기존 XV6에서는 적은 디스크 블록을 요구하는 파일크기를 지원하며 할당한 가상페이지 크기만큼 물리 페이지 크기를 할당했는데)
  추가로 가상 페이지를 할당하는 시스템 콜을 추가(과제2 했으면 쉬움)시켜서, 가상 페이지는 있는데 물리 페이지(디스크 블록이 메모리에 올라와있지 않은 경우) == 페이지 폴트 인터럽트 처리를 구현해야했다.

- 기존 XV6 파일시스템에서 지원하는 inode 구조체가 가리키는 디스크 블록의 트리를 변경하여서, 큰 파일을 지원하도록 구현해야했다.
  얘도 기존 코드 분석이 요구됬다. 당연하게도... 어떤 함수들이 가상 페이지 / 물리 페이지 / 디스크 블록 과 관련된 함수인지 어떤 함수들을 손봐야하는지 

- inode 구조체에 만들 파일의 크기만큼의 디스크 블록 할당을 위해 bmap() 함수 / itrunc() 함수를 새로 짜야했다.
  이 과정중, 인덱스 계산과 같은 것들이 조금 어려웠다.
  
