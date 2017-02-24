/*
	MornitoringSystem Client

		���� : 2010154041 ������
		���� : 2013150023 ������
			   2013150034 ������

								�� �����Ͽ����ϴ�.
*/
#include <stdio.h>
#include <stdlib.h>
#include <winsock2.h>	
#include <conio.h>
#include <time.h>		// �����Լ� ����ϱ����� ���
#include <windows.h>	// ������ ����ϱ����� ��� 
#include <process.h>	// ������ ����ϱ����� ���

// ����
#define SERV_IP				"127.0.0.1"		// ���� ������
#define SERV_PORT			8282			// ���� ��Ʈ
//---------------------- �輭�� ���� -----------------------
#define STATUS_PAUSE		0				// ������´� 0
#define STATUS_RUN			1				// �������� ���´� 1
#define STATUS_EXIT			2				// ����� ���´� 2
#define STATUS_FULL_QUEUE	3				// ť�� ��á����� 3
//---------------------- ����� ���� -----------------------
#define BUF_MAX_SIZE		2				// ������ �ְ���� buf ũ�� 2
#define	BUF_STATUS			0				// buf���� ���°��� ���ִ� ��ġ 0
#define BUF_SENSOR			1				// buf���� �������� ���ִ� ��ġ 1
//-------------------- ��buf ���� ���� ---------------------
#define RE_RUN				1
#define	EXIT				2
//---------------------- �谢�� ���� -----------------------

// �Լ�����
void PrintError(char *msg);							// ���� ȣ�� �Լ�
void PrintIndex(void);								// �ε��� ȭ�� ��� �Լ�

// �������� ����
SOCKET cHandle;					// Ŭ���̾�Ʈ �ڵ�
SOCKADDR_IN	sAddr;				// ���ϱ���ü �����ּ�
char buf[BUF_MAX_SIZE];			// ������ �ְ���� buf



int main()
{
	// ���� ���̺귯�� �ʱ�ȭ
	WSADATA	wsa;
	WSAStartup( MAKEWORD(2,2), &wsa );
	
	// ���� ���� - UDP
	cHandle = socket( PF_INET, SOCK_DGRAM, 0);
	if( cHandle == INVALID_SOCKET )
		PrintError("UDP ���� ���� ����");


	// ���� �ּҼ���
	memset(&sAddr, 0, sizeof(sAddr) );
	sAddr.sin_family		=	AF_INET;
	sAddr.sin_addr.s_addr	=	inet_addr(SERV_IP);
	sAddr.sin_port			=	htons(SERV_PORT);

	// -------------------------------- ����� ���� UDP Ŭ���̾�Ʈ ���� �Ϸ� --------------------------------



	srand((unsigned)time(NULL));	// �������� ��� �ٲ�� ����

	int time;	// ������ ���� ������
	int ret;	// ���ϰ� Ȯ�� ����

	PrintIndex();		// �ε���ȭ�� ���

	fflush(stdin);
	printf("> �� �ʸ��� �������� �����ðڽ��ϱ�? ");
	scanf("%d",&time);

	int cmd = 0;
	buf[BUF_STATUS] = STATUS_RUN;
	PrintIndex();
	while(1)	{
		buf[BUF_SENSOR] = rand()%100;	// 1~100 ������ ���� buf[0 (=SENSOR) ]�� ����
			
		printf("���� SENSOR�� = %d \n", buf[BUF_SENSOR]);

		ret = sendto( cHandle, buf, sizeof(buf), 0, (SOCKADDR *)&sAddr  , sizeof(sAddr) );	// ������ �� Ŭ���̾�Ʈ���� ������ ����
	
		
		recvfrom(cHandle, buf,sizeof(buf),0, NULL, 0);
		if( buf[BUF_STATUS] == STATUS_PAUSE )	{
			PrintIndex();
			printf("\n");
			printf("���� SENSOR�� = %d \t\t [ P A U S E ] \n", buf[BUF_SENSOR]);
			printf("�ٽ� �����Ͻðڽ��ϱ�? ( 1:�簡�� / 2:���� )  : ");
			scanf("%d",&cmd);
				switch(cmd)	{
					case RE_RUN:
						// �簡�� :	1
						buf[BUF_STATUS] = STATUS_RUN;
						break;
					case EXIT:
						// ����	  :	2
						buf[BUF_STATUS] = STATUS_EXIT;
						sendto( cHandle, buf, sizeof(buf), 0, (SOCKADDR *)&sAddr  , sizeof(sAddr) );	// ������ ������ ����
						printf("�̿����ּż� �����մϴ�. �����ϰڽ��ϴ�. \n");
						return 0;
				}
		}
		else if( buf[BUF_STATUS] == STATUS_EXIT )	{
			printf(">>> ������ ����Ǿ����ϴ�. ���� ���� \n");
			break;
		}
		else if(buf[BUF_STATUS] == STATUS_FULL_QUEUE )	{
			printf(">>> ���� ����͸� ���� Ŭ���̾�Ʈ�� ������ ���Ӱźδ��߽��ϴ�. ���۽��� \n");
			break;
		}
		else
			printf(">>> ������ ���� �Ϸ� \n");

		
		printf("\n\n%d�� �Ŀ� �������� �����մϴ�. \n",time);
		Sleep(time * 1000);		// �Է¹��� time(��) ���� ������ ����
	
		PrintIndex();
		printf("���� SENSOR�� = %d \n", buf[BUF_SENSOR]);
		
	}

	getch();
	return 0;
}

// �����޼��� ����Լ�
void PrintError(char *msg)	{
	printf(">>>[ERROR] %s \n",msg);
	exit(0);
}
// �ε���ȭ�� ��� �Լ�
void PrintIndex(void)	{
	system("cls");
	printf("������������������������������������������������������������������������������\n");
	printf("��                                                                          ��\n");
	printf("��                                                                          ��\n");
	printf("��                 Mornitoring                                              ��\n");
	printf("��                                        System  (Client)                  ��\n");
	printf("��                                                                          ��\n");
	printf("��                                                                          ��\n");
	printf("������������������������������������������������������������������������������\n\n");
}