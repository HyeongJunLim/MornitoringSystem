/*
	MornitoringSystem Server

		팀장 : 2010154041 임형준
		팀원 : 2013150023 윤유진
			   2013150034 정현선

								이 구현하였습니다.
*/
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <conio.h>
#include <winsock2.h>	
#include <windows.h>	// 쓰레드 사용하기 위한 헤더 
#include <process.h>	// 쓰레드 사용하기 위한 헤더
#include <ws2tcpip.h>	// 멀티캐스트 사용하기 위한 헤더 

// 정의
#define SERV_IP				"127.0.0.1"	// 서버 아이피
#define SERV_PORT			8282			// 서버 포트
#define FILE_ADDRESS		"DATA\\"		// 파일들 저장될 위치
//---------------------- ↑서버 설정 -----------------------
#define STATUS_PAUSE		0				// 멈춘상태는 0
#define STATUS_RUN			1				// 진행중인 상태는 1
#define STATUS_EXIT			2				// 종료된 상태는 2
#define STATUS_FULL_QUEUE	3				// 큐가 꽉찼을경우 3
//---------------------- ↑상태 정의 -----------------------
#define BUF_MAX_SIZE		2				// 서버와 주고받을 buf 크기 2
#define	BUF_STATUS			0				// buf에서 상태값이 들어가있는 위치 0
#define BUF_SENSOR			1				// buf에서 센서값이 들어가있는 위치 1
//-------------------- ↑buf 관련 정의 ---------------------
#define CLIENT_MAX_NUM		10				// 클라이언트 최대갯수 10
#define NODE_MAX_NUM		100				// 노드 최대갯수 100
#define SENSOR_LIMIT		90				// 센서 한계 90
//---------------------- ↑각종 값들 -----------------------


// 구조체 정의
typedef struct double_list_queue *queue_pointer;
typedef struct queue_head *head_pointer;

//구조체 선언
struct double_list_queue	{
	int				sensor;
	int				status;
	queue_pointer	left_link, right_link;
};
struct queue_head	{
	queue_pointer	right_link;					
	int				now_status;							// 현재 상태를 저장
	int				now_sensor;							// 현재 센서값 저장
	int				sum_sensor;							// 센서값 총합을 저장하기위한 변수
	int				cnt_sensor;							// 센서 카운트 변수
	double			ave_sensor;							// 평균 센서값 저장하는 변수
	int				min_sensor;							// 저장된 센서값중 가장 작은 센서값 저장하는 변수
	int				max_sensor;							// 저장된 센서값중 가장 큰 센서값 저장하는 변수
	char			ip[20];								// 접속한 클라이언트 ip 주소 저장을 위한 변수
	int				level[10];							// 도수분포표의 계급의 도수를 count하기 위한 배열
};


// 함수 선언
void initialize_queue( head_pointer quit_queue );			// 모든 큐 초기화 or 종료된 클라이언트의 큐 초기화
int insert_queue( char *ip, int sensor );					// 큐 삽입
void PrintError( char *msg );								// 에러메세지 출력
void PrintIndex( void );									// 인덱스화면 출력
int SearchID( char *ip );									// 들어갈 큐 위치찾기
void SensorMaxMin( head_pointer temp );						// 센서의 최대, 최소값 구해서 큐에 저장
void SensorGraph( head_pointer queue );						// 센서의 도수분포표 구하기
void PrintInfo( head_pointer queue );						// 센서에 대한 정보 출력
unsigned WINAPI ThreadFunc( void *arg );					// 쓰레드 함수
int AllClientInfo ( void );									// 접속된 모든 클라이언트의 정보
void PrintMenu(void);										// 메뉴 함수
void DetailClientInfo();

	
// 전역변수 설정
SOCKET sHandle;							// 클라이언트 핸들
SOCKADDR_IN	sAddr;						// 소켓구조체 서버주소
SOCKADDR_IN	cAddr;						// 소켓구조체 클라이언트
char buf[BUF_MAX_SIZE];					// 클라이언트와 주고받기위한 buf배열
char cmd;								// 메뉴선택 변수
int type;								// 타입 설정
int id;									// 세부정보 볼때 필요한 변수
FILE	*fp;							// 파일 포인터
HANDLE	delay, menu;					// 세마포어, 이벤트 이용 핸들

head_pointer	queue[CLIENT_MAX_NUM];
	
	


int main(void)
{
	// 소켓 라이브러리 초기화
	WSADATA	wsa;
	WSAStartup( MAKEWORD(2,2), &wsa );
	
	// 소켓 생성 - UDP
	sHandle = socket( PF_INET, SOCK_DGRAM, 0);
	if( sHandle == INVALID_SOCKET )
		PrintError("UDP 소켓 생성 실패");

	// 서버 주소설정
	memset(&sAddr, 0, sizeof(sAddr) );
	
	sAddr.sin_family		=	AF_INET;
	sAddr.sin_addr.s_addr	=	inet_addr(SERV_IP);
	sAddr.sin_port			=	htons(SERV_PORT);

	
	bind( sHandle, (SOCKADDR *)&sAddr, sizeof(sAddr) );
	// -------------------------------- 통신을 위한 UDP 서버 설정 완료 --------------------------------
	HANDLE	hThread;		// 스레드 핸들	
	unsigned threadID;		// 쓰레드ID

	
	initialize_queue(NULL);	// 리스트 초기화
	


	printf(" > 클라이언트 접속을 기다리는중입니다... \n");
	
	delay	= CreateSemaphore(NULL, 1, 1, NULL);			// 세마포어 생성
	menu	= CreateEvent(NULL, TRUE, FALSE, NULL);			// 이벤트 생성

	hThread = (HANDLE)_beginthreadex( NULL, 0, ThreadFunc, 0, 0, &threadID );	// 쓰레드 생성

	while(1)	{
		WaitForSingleObject(menu, INFINITE);


		scanf("%c",&cmd);					// 메뉴 선택

		fflush(stdin);

		int cnt = 0;
		//--------- 모든 클라이언트 접속종료되었을때 메뉴선택 X-------
		for(int i=0; i<CLIENT_MAX_NUM; i++)	{
			if(queue[i]->right_link == NULL)
				cnt++;
		}
		if(cnt == CLIENT_MAX_NUM)	{
			ResetEvent(menu);
			system("cls");
			printf(" > 모든 클라이언트가 접속 종료되었습니다... \n");
			printf(" > 클라이언트 접속을 기다리는중입니다... \n");
			continue;
		}
		//--------------------------------------------------------------

		switch(cmd)	{
			case '1':
				// 클라이언트 세부정보 열기
				WaitForSingleObject(delay, INFINITE);
				
				type = 1;

				printf(">> 클라이언트를 선택해주세요(1 ~ %d) : ", CLIENT_MAX_NUM);
				scanf("%d",&id);

				while( id > CLIENT_MAX_NUM || id < 1 || queue[id-1]->right_link == NULL )	{
					printf("[ ERROR ] 유효하지 않은 클라이언트 넘버입니다. 다시입력해주세요. \n");
					printf(">> 클라이언트를 선택해주세요(1 ~ %d) : ", CLIENT_MAX_NUM);
					scanf("%d",&id);
				}
				id--;
			
				ReleaseSemaphore(delay, 1, NULL);
				break;

			case '2':
				// 클라이언트 세부정보 닫기
				type = 0;
				break;
	
			case 'q':
				// 프로그램 종료하기
				WaitForSingleObject(hThread,INFINITE);
				PrintIndex();							// 인덱스화면 출력
				printf("\n\n");
				printf(" >>> 프로그램 종료 준비 중입니다. 기다려주세요... \n");
							
				break;
		}

		if(cmd == 'q')
			break;
	}
	

	system("cls");
	printf(" >>> 프로그램 정상 종료 완료... \n");
	getch();
	return 0;
}





// 큐 초기화 함수
void initialize_queue(head_pointer quit_queue)	{

	if(buf[BUF_STATUS] == STATUS_EXIT)	{
		// 종료된 클라이언트가 들어가있는 큐만 초기화
		
		
		free(quit_queue);						// 메모리 초기화

		// 재생성 ---------------------------------------------------
		quit_queue = (head_pointer)malloc(sizeof(struct queue_head));
		quit_queue->right_link = NULL;
		quit_queue->now_status = 0;									// 현재 상태를 저장하는 변수 초기화
		quit_queue->now_sensor = 0;									// 현재 센서값을 저장하는 변수 초기화
		quit_queue->sum_sensor = 0;									// 센서값 저장하는 변수 초기화
		quit_queue->cnt_sensor = 0;									// 센서값 카운트 변수 초기화
		quit_queue->ave_sensor = 0.0;								// 평균 센서값 저장하는 변수 초기화
		quit_queue->min_sensor = 0;									// 최소 센서값 저장하는 변수 초기화
		quit_queue->max_sensor = 0;									// 최대 센서값 저장하는 변수 초기화
		memset(quit_queue->ip, NULL, sizeof(quit_queue->ip));		// ip주소 저장하는 char 배열 초기화
		memset(quit_queue->level, 0 , sizeof(quit_queue->level));	// 계급의 도수를 저장하는 int 배열 초기화
	}
	else	{
		// 모든 큐 초기화
		for(int i=0; i<CLIENT_MAX_NUM; i++)
		{
			queue[i] = (head_pointer)malloc(sizeof(struct queue_head));
			queue[i]->right_link = NULL;
			queue[i]->now_status = 0;									// 현재 상태를 저장하는 변수 초기화
			queue[i]->now_sensor = 0;									// 현재 센서값을 저장하는 변수 초기화
			queue[i]->sum_sensor = 0;									// 센서값 저장하는 변수 초기화
			queue[i]->cnt_sensor = 0;									// 센서값 카운트 변수 초기화
			queue[i]->ave_sensor = 0.0;									// 평균 센서값 저장하는 변수 초기화
			queue[i]->min_sensor = 0;									// 최소 센서값 저장하는 변수 초기화
			queue[i]->max_sensor = 0;									// 최대 센서값 저장하는 변수 초기화
			memset(queue[i]->ip, NULL, sizeof(queue[i]->ip));			// ip주소 저장하는 char 배열 초기화
			memset(queue[i]->level, 0 , sizeof(queue[i]->level));		// 계급의 도수를 저장하는 int 배열 초기화
			
		}
	}
}
// 큐 삽입 함수
int insert_queue(char *ip, int sensor)	{
	queue_pointer new_node = (queue_pointer)malloc(sizeof(struct double_list_queue));	// 새노드
	if(new_node == NULL)
		PrintError("메모리 할당 에러");

	// ---------------------------- ↓큐 헤드 작업 ---------------------------------
	int ID = SearchID(ip);					// 들어갈 큐 찾기

	if(ID == -1)	{
		// 모든 큐가 꽉차서 할당 못받았을경우
		buf[BUF_STATUS] = STATUS_FULL_QUEUE;
		sendto( sHandle, buf, sizeof(buf), 0, (SOCKADDR*)&cAddr, sizeof(cAddr) );	// 클라이언트로 전송
			
		return 0;
	}


	queue[ID]->now_sensor = sensor;														// 해당큐의 현재센서값 변수에 들어온 센서값저장
	queue[ID]->cnt_sensor++;															// 해당큐의 센서 카운트변수 1 증가
	queue[ID]->sum_sensor += sensor;													// 해당큐의 센서값 총합에 센서값 저장
	queue[ID]->ave_sensor = (double)queue[ID]->sum_sensor/queue[ID]->cnt_sensor;		// 해당큐의 평균값 저장
	SensorMaxMin( queue[ID] );															// 해당큐의 센서값의 최댓값 최솟값 구해서 queue[ID]의 변수에 저장


	fseek(fp, 0, SEEK_END);	// 파일 맨끝으로 이동
	fprintf(fp, "%d \t %d \t\t %d \t\t %d \t\t %.2lf \n",queue[ID]->cnt_sensor, queue[ID]->now_sensor, queue[ID]->max_sensor, queue[ID]->min_sensor, queue[ID]->ave_sensor);
	fclose(fp);				// 파일 입력종료
	
	// ---------------------------- ↓새 노드 작업 ---------------------------------
	new_node->right_link = NULL;
	new_node->sensor = sensor;
	if( sensor > SENSOR_LIMIT || sensor == SENSOR_LIMIT )	{	// 센서값 >= 80 이면
		queue[ID]->now_status = STATUS_PAUSE;					// 큐의 현재상태 PAUSE
		new_node->status = STATUS_PAUSE;						// 상태 PAUSE( = 0 )
		buf[BUF_STATUS] = STATUS_PAUSE;							// 상태 PAUSE( = 0 )
		buf[BUF_SENSOR] = queue[ID]->now_sensor;				// 버퍼 센서에 들어온 센서값 넣어줌
		sendto( sHandle, buf, sizeof(buf), 0, (SOCKADDR*)&cAddr, sizeof(cAddr) );	// 클라이언트로 전송
	}
	else	{													// 센서값 < 80 이면
		queue[ID]->now_status = STATUS_RUN;						// 큐의 현재상태 RUN
		new_node->status = STATUS_RUN;							// 상태 RUN ( = 1 )
		buf[BUF_STATUS] = STATUS_RUN;							// 상태 PAUSE( = 0 )
		buf[BUF_SENSOR] = queue[ID]->now_sensor;				// 버퍼 센서에 들어온 센서값 넣어줌
		sendto( sHandle, buf, sizeof(buf), 0, (SOCKADDR*)&cAddr, sizeof(cAddr) );	// 클라이언트로 전송
	}
	// -----------------------------------------------------------------------------
	


	if( queue[ID]->right_link == NULL )	{
		// 첫 노드일때의 큐 삽입
		queue[ID]->right_link = new_node;
	}
	else	{	
		// 첫노드가 아닐때의 큐 삽입

		// --------------------------- ↓맨 앞 노드 삭제 -------------------------------
		if( queue[ID]->cnt_sensor % NODE_MAX_NUM == 1 )	{
			// 센서카운트를 NODE_MAX_NUM으로 나누었을때 1이면, 예를들어 NODE_MAX_NUM가 100일때 101번째 노드이면 맨 앞 노드 삭제
			// (첫노드는 신경쓸필요 없음. 이 부분엔 접근하지 않으므로.)
			queue_pointer	delete_temp;			// 맨 앞 노드 삭제용 delete_temp
			delete_temp = queue[ID]->right_link;
			
			queue[ID]->right_link = delete_temp->right_link;
			queue[ID]->right_link->left_link = NULL;
			free(delete_temp);
		}
		// --------------------------- ↓맨 끝 노드에 추가 -----------------------------
		queue_pointer move_temp;					// 맨 끝 노드로 이동용 move_temp
		move_temp = queue[ID]->right_link;

		while( move_temp->right_link != NULL )	{
			//맨 끝노드로 이동
			move_temp = move_temp->right_link;
		}
		move_temp->right_link = new_node;			// 새노드 추가
		new_node->left_link = move_temp;			// 새노드랑 마지막 노드 연결
		//------------------------------------------------------------------------------
	}

	WaitForSingleObject(delay,INFINITE);
	// --------------------- ↓큐에 삽입 완료후 여러가지 작업 ----------------------
	SensorGraph( queue[ID] );				// 해당큐의 센서들의 분포를 확인 할수 있는 표 생성

	PrintIndex();							// 인덱스화면 출력
	PrintInfo( queue[ID] );					// 센서값에 대한 정보를 출력
	AllClientInfo();						// 모든 클라이언트 정보 출력

	if(type == 1)							// 클라이언트 자세한정보 보고싶다는 요청을 받았을때
		DetailClientInfo();					// 함수호출
	
	PrintMenu();							// 메뉴 출력
	
	SetEvent(menu);						// 메뉴 출력후 메뉴 입력가능
	//------------------------------------------------------------------------------
	ReleaseSemaphore(delay, 1, NULL);

	return 0;
}

// 인덱스화면 출력 함수
void PrintIndex(void)	{
	system("cls");
	printf("┏━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┓\n");
	printf("┃                                                                          ┃\n");
	printf("┃                                                                          ┃\n");
	printf("┃                 Mornitoring                                              ┃\n");
	printf("┃                                        System  (Server)                  ┃\n");
	printf("┃                                                                          ┃\n");
	printf("┃                                                                          ┃\n");
	printf("┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┛\n\n");
}
// 들어갈 큐찾는 함수
int SearchID(char *ip)	{
	int id = -1 ;

	char file_name[50];
	
	for(int i=0; i<CLIENT_MAX_NUM; i++)	{
		if( !strcmp(queue[i]->ip,ip) )	{
			// 동일한 아이피가 있는지 검색
			sprintf(file_name, "%sCLIENT %s.txt", FILE_ADDRESS ,ip);
			//sprintf(file_name, "CLIENT %s.txt", ip);

			fp = fopen(file_name, "r+");

			id = i;		// id에 i값 저장
			break;
		}
	}

	if( id == -1 )	{
		// 검색된 동일한 아이피주소가 없을때 새로 추가
		for(int i=0; i<CLIENT_MAX_NUM; i++)	{
			if( *queue[i]->ip == NULL  )	{
				strncpy(queue[i]->ip, ip,strlen(ip));		// queue->ip에 복사
				
				sprintf(file_name, "%sCLIENT %s.txt", FILE_ADDRESS ,ip);

				fp = fopen(file_name, "w+");
				fprintf(fp, "Client IP = %s \n\n",ip);
				fprintf(fp, "Count \t Sensor \t Max_Sensor \t Min_Sensor \t Average_Sensor \n");
							
				id = i;
				break;
			}
		}
	}
	


	return id;
}

// 에러메세지 출력 함수
void PrintError(char *msg)	{
	printf("> [ERROR] %s \n",msg);
	exit(0);
}
// 센서의 최대, 최소값 계산후 저장
void SensorMaxMin( head_pointer queue  )	{
		
	if( queue->cnt_sensor == 1 )	{
		// 노드가 한개밖에없을때
		queue->max_sensor = queue->now_sensor;		// 그 큐의 최댓값에 센서값 저장
		queue->min_sensor = queue->now_sensor;		// 그 큐의 최솟값에 센서값 저장
 	}

	else	{
		// 노드가 두 개 이상일때
		if(queue->max_sensor < queue->now_sensor )		// 그 큐의 최댓값이 방금 들어온 센서값보다 작으면 변경 
			queue->max_sensor = queue->now_sensor;
		else
			queue->max_sensor;
		
		if(queue->min_sensor > queue->now_sensor )		// 그 큐의 최솟값이 방금 들어온 센서값보다 크면 변경
			queue->min_sensor = queue->now_sensor;
		else
			queue->min_sensor;
	}
}

// 해당 큐의 센서들의 분포를 확인 할수 있는 표 생성
void SensorGraph(head_pointer queue)
{
	queue_pointer last_node;
	
	int temp;
	
	last_node = queue->right_link;

	temp = queue->now_sensor / 10;	// 센서의 계급 계산
	queue->level[temp]++;			// 해당 계급의 도수를 1씩 증가
	
}

// 해당 큐의 정보 & 모든 큐의 정보 & 메뉴를 보는함수
void PrintInfo(head_pointer enter_queue)	{
	if(enter_queue->now_status != STATUS_EXIT)	{
		// 종료되서 호출된게 아닐때
		printf("\t   ┌──────────────────────────┐\n");
		printf("\t   │\t\tLately\t\t\t\t\t │\n");
		printf("\t   │\t\t\tClient [ %s ]\t │\n", enter_queue->ip);
		printf("\t   │\t\t\t\t\t\t\t │\n");
		printf("\t   │\t\t 센서값 : %d \t\t\t\t │\n", enter_queue->now_sensor);
		printf("\t   │\t\t 최댓값 : %d \t\t\t\t │\n", enter_queue->max_sensor);
		printf("\t   │\t\t 최솟값 : %d \t\t\t\t │\n", enter_queue->min_sensor);
		printf("\t   │\t\t 평균값 : %.2lf \t\t\t │\n", enter_queue->ave_sensor);
		printf("\t   └──────────────────────────┘\n");
	}
	else	{
		// 종료되서 호출되었을때
		printf("\t   ┌──────────────────────────┐\n");
		printf("\t   │\t\tLately\t\t\t\t\t │\n");
		printf("\t   │\t\t\tClient [ %s ]\t │\n", enter_queue->ip);
		printf("\t   │\t\t\t\t\t\t\t │\n");
		printf("\t   │\t\t\t\t\t\t\t │\n");
		printf("\t   │\t\t     접   속   종   료\t\t\t │\n");
		printf("\t   │\t\t\t\t\t\t\t │\n");
		printf("\t   │\t\t\t\t\t\t\t │\n");
		printf("\t   └──────────────────────────┘\n");
	}

}
// 전체 정보 출력 함수
int AllClientInfo(void)	{
	printf("\n\n");
	printf(" ────────────[ 모든 접속 클라이언트정보 ]──────────── \n");
	printf(" NUM \t IP \t\t LATELY_SENSOR \t AVERAGE_SENSOR \t STATUS \n");
	for(int i=0; i<CLIENT_MAX_NUM; i++)	{
	
		if( queue[i]->right_link != NULL )	{
			// 첫노드가 연결되었을경우,즉 클라이언트로부터 하나라도 센서값을 받았을경우
			switch(queue[i]->now_status)	{
				case STATUS_RUN:
					printf(" %d \t %s\t %d \t\t %.2lf \t\t\t [ R U N ] \n", i+1, queue[i]->ip, queue[i]->now_sensor, queue[i]->ave_sensor);
					break;
				case STATUS_PAUSE:
					printf(" %d \t %s\t %d \t\t %.2lf \t\t\t [ P A U S E ] \n", i+1, queue[i]->ip, queue[i]->now_sensor, queue[i]->ave_sensor);
					break;
				case STATUS_EXIT:
					printf(" %d \t %s\t %d \t\t %.2lf \t\t\t [ E X I T ] \n", i+1, queue[i]->ip, queue[i]->now_sensor, queue[i]->ave_sensor);
					break;
			}
		}
		else
			printf(" %d \t\t\t    [ NOT CONNECTED ] \n", i+1);
	}
	printf(" ────────────────────────────────────── \n\n");
		



	return 0;
}
// 메뉴 함수
void PrintMenu(void)	{

	printf("\n");
	printf("[ 메 뉴 ] ───────────────────────────────── \n");
	printf("  1) 클라이언트 정보 자세히 보기 \n");
	printf("  2) 클라이언트 정보 자세히 보기 닫기 \n");
	printf("  q) 종료하기 \n\n");
	printf("> 선택하세요 :  ");
	
}
// 선택한 클라이언트 정보 자세히보는 함수
void DetailClientInfo()	{
	int cnt=0;

	if(queue[id]->now_status == STATUS_EXIT)	{
		printf("\t   ┌──────────────────────────┐\n");
		printf("\t   │\t\tSelected\t\t\t\t │\n");
		printf("\t   │\t\t\tClient [ %s ]\t │\n", queue[id]->ip);
		printf("\t   │\t\t\t\t\t\t\t │\n");
		printf("\t   │\t\t\t\t\t\t\t │\n");
		printf("\t   │\t\t     접   속   종   료\t\t\t │\n");
		printf("\t   │\t\t\t\t\t\t\t │\n");
		printf("\t   │\t\t\t\t\t\t\t │\n");
		printf("\t   └──────────────────────────┘\n");
		type=0;
	}
	else	{
		printf("\t   ┌──────────────────────────┐\n");
		printf("\t   │\t\tSelected\t\t\t\t │\n");
		printf("\t   │\t\t\tClient [ %s ]\t │\n", queue[id]->ip);
		printf("\t   │\t\t\t\t\t\t\t │\n");
		printf("\t   │\t\t 센서값 : %d \t\t\t\t │\n", queue[id]->now_sensor);
		printf("\t   │\t\t 최댓값 : %d \t\t\t\t │\n", queue[id]->max_sensor);
		printf("\t   │\t\t 최솟값 : %d \t\t\t\t │\n",	queue[id]->min_sensor);
		printf("\t   │\t\t 평균값 : %.2lf \t\t\t │\n", queue[id]->ave_sensor);
		printf("\t   └──────────────────────────┘\n");

		printf("\n\t       ─────────[ 도수 분포표 ]───────── \n");
		for(int i=0; i < 10; i++)	{
			// 도수분포표 출력
			printf("\t\t\t%d ~ %d\t｜", cnt, cnt+9);

			for(int j=0; j<queue[id]->level[i]; j++){
				printf("■");
			}
			cnt += 10;
			printf("\n");

		}
		printf("\t      ──────────────────────────\n\n");
	}
}

// 쓰레드 함수
unsigned WINAPI ThreadFunc(void *arg)	{
	int cSize;
	int ID;
	cSize = sizeof(cAddr);

	while(cmd != 'q' )	{
		recvfrom( sHandle, buf , sizeof(buf), 0, (SOCKADDR *)&cAddr, &cSize);

		if(buf[BUF_STATUS] == STATUS_EXIT)	{
			// 클라이언트로부터 받은 BUF_STATUS가 2, 즉 종료처리이면
			ID= SearchID( inet_ntoa(cAddr.sin_addr) );
			queue[ID]->now_status = STATUS_EXIT;

			WaitForSingleObject(delay,INFINITE);
			//-----------------------------------------------------------------------------
			PrintIndex();							// 인덱스화면 출력
			PrintInfo( queue[ID] );					// 큐 정보 출력
			AllClientInfo();						// 모든 클라이언트 정보 출력
			if(type == 1)							// 클라이언트 자세한정보 보고싶다는 요청을 받았을때
				DetailClientInfo();					// 함수호출
			PrintMenu();							// 메뉴 출력
			//-----------------------------------------------------------------------------
			ReleaseSemaphore(delay, 1, NULL);
		
			initialize_queue( queue[ID] );			// 큐 초기화 진행

			int cnt=0;
			for(int i=0; i<CLIENT_MAX_NUM; i++)	{
				if(queue[i]->right_link == NULL)
					cnt++;
			}
			if(cnt == CLIENT_MAX_NUM)	{
				Sleep(2 * 1000);
				system("cls");
				printf(" > 모든 클라이언트가 접속 종료되었습니다... \n");
				printf(" > 클라이언트 접속을 기다리는중입니다... \n");
			}
				
		}
		else
			// 클라이언트로부터 받은 BUF_STATUS가 2가아닌경우, 런 상태이면
			insert_queue( inet_ntoa(cAddr.sin_addr), buf[BUF_SENSOR] );
	}

	while(cmd == 'q')	{
		// 종료메뉴를 선택받았을시
		recvfrom( sHandle, buf , sizeof(buf), 0, (SOCKADDR *)&cAddr, &cSize);
		
		ID= SearchID( inet_ntoa(cAddr.sin_addr) );

		buf[BUF_STATUS] = STATUS_EXIT;							// 상태 EXIT( = 2 )
		queue[ID]->now_status = STATUS_EXIT;

		initialize_queue( queue[ID] );							// 큐 초기화 진행

			
		sendto( sHandle, (char *)&buf[BUF_STATUS], sizeof(buf[BUF_STATUS]), 0, (SOCKADDR*)&cAddr, sizeof(cAddr) );	// 서버 종료되었다는 메세지 보내기

		int cnt=0;
		for(int i=0; i<CLIENT_MAX_NUM; i++)	{
			if(queue[i]->right_link == NULL)
				cnt++;
		}
		if(cnt == CLIENT_MAX_NUM)	
			break;		

	}



	return 0;
}
