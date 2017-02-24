/*
	MornitoringSystem Client

		팀장 : 2010154041 임형준
		팀원 : 2013150023 윤유진
			   2013150034 정현선

								이 구현하였습니다.
*/
#include <stdio.h>
#include <stdlib.h>
#include <winsock2.h>	
#include <conio.h>
#include <time.h>		// 랜덤함수 사용하기위한 헤더
#include <windows.h>	// 쓰레드 사용하기위한 헤더 
#include <process.h>	// 쓰레드 사용하기위한 헤더

// 정의
#define SERV_IP				"127.0.0.1"		// 서버 아이피
#define SERV_PORT			8282			// 서버 포트
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
#define RE_RUN				1
#define	EXIT				2
//---------------------- ↑각종 값들 -----------------------

// 함수선언
void PrintError(char *msg);							// 에러 호출 함수
void PrintIndex(void);								// 인덱스 화면 출력 함수

// 전역변수 설정
SOCKET cHandle;					// 클라이언트 핸들
SOCKADDR_IN	sAddr;				// 소켓구조체 서버주소
char buf[BUF_MAX_SIZE];			// 서버와 주고받을 buf



int main()
{
	// 소켓 라이브러리 초기화
	WSADATA	wsa;
	WSAStartup( MAKEWORD(2,2), &wsa );
	
	// 소켓 생성 - UDP
	cHandle = socket( PF_INET, SOCK_DGRAM, 0);
	if( cHandle == INVALID_SOCKET )
		PrintError("UDP 소켓 생성 실패");


	// 서버 주소설정
	memset(&sAddr, 0, sizeof(sAddr) );
	sAddr.sin_family		=	AF_INET;
	sAddr.sin_addr.s_addr	=	inet_addr(SERV_IP);
	sAddr.sin_port			=	htons(SERV_PORT);

	// -------------------------------- 통신을 위한 UDP 클라이언트 설정 완료 --------------------------------



	srand((unsigned)time(NULL));	// 랜덤숫자 계속 바뀌게 설정

	int time;	// 센서값 보낼 딜레이
	int ret;	// 리턴값 확인 변수

	PrintIndex();		// 인덱스화면 출력

	fflush(stdin);
	printf("> 몇 초마다 센서값을 보내시겠습니까? ");
	scanf("%d",&time);

	int cmd = 0;
	buf[BUF_STATUS] = STATUS_RUN;
	PrintIndex();
	while(1)	{
		buf[BUF_SENSOR] = rand()%100;	// 1~100 사이의 숫자 buf[0 (=SENSOR) ]에 저장
			
		printf("현재 SENSOR값 = %d \n", buf[BUF_SENSOR]);

		ret = sendto( cHandle, buf, sizeof(buf), 0, (SOCKADDR *)&sAddr  , sizeof(sAddr) );	// 센서값 및 클라이언트상태 서버로 전송
	
		
		recvfrom(cHandle, buf,sizeof(buf),0, NULL, 0);
		if( buf[BUF_STATUS] == STATUS_PAUSE )	{
			PrintIndex();
			printf("\n");
			printf("현재 SENSOR값 = %d \t\t [ P A U S E ] \n", buf[BUF_SENSOR]);
			printf("다시 가동하시겠습니까? ( 1:재가동 / 2:종료 )  : ");
			scanf("%d",&cmd);
				switch(cmd)	{
					case RE_RUN:
						// 재가동 :	1
						buf[BUF_STATUS] = STATUS_RUN;
						break;
					case EXIT:
						// 종료	  :	2
						buf[BUF_STATUS] = STATUS_EXIT;
						sendto( cHandle, buf, sizeof(buf), 0, (SOCKADDR *)&sAddr  , sizeof(sAddr) );	// 센서값 서버로 전송
						printf("이용해주셔서 감사합니다. 종료하겠습니다. \n");
						return 0;
				}
		}
		else if( buf[BUF_STATUS] == STATUS_EXIT )	{
			printf(">>> 서버가 종료되었습니다. 전송 실패 \n");
			break;
		}
		else if(buf[BUF_STATUS] == STATUS_FULL_QUEUE )	{
			printf(">>> 현재 모니터링 중인 클라이언트가 꽉차서 접속거부당했습니다. 전송실패 \n");
			break;
		}
		else
			printf(">>> 서버에 전송 완료 \n");

		
		printf("\n\n%d초 후에 센서값을 전송합니다. \n",time);
		Sleep(time * 1000);		// 입력받은 time(초) 마다 센서값 전송
	
		PrintIndex();
		printf("이전 SENSOR값 = %d \n", buf[BUF_SENSOR]);
		
	}

	getch();
	return 0;
}

// 에러메세지 출력함수
void PrintError(char *msg)	{
	printf(">>>[ERROR] %s \n",msg);
	exit(0);
}
// 인덱스화면 출력 함수
void PrintIndex(void)	{
	system("cls");
	printf("┏━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┓\n");
	printf("┃                                                                          ┃\n");
	printf("┃                                                                          ┃\n");
	printf("┃                 Mornitoring                                              ┃\n");
	printf("┃                                        System  (Client)                  ┃\n");
	printf("┃                                                                          ┃\n");
	printf("┃                                                                          ┃\n");
	printf("┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┛\n\n");
}