/*
	MornitoringSystem Server

		���� : 2010154041 ������
		���� : 2013150023 ������
			   2013150034 ������

								�� �����Ͽ����ϴ�.
*/
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <conio.h>
#include <winsock2.h>	
#include <windows.h>	// ������ ����ϱ� ���� ��� 
#include <process.h>	// ������ ����ϱ� ���� ���
#include <ws2tcpip.h>	// ��Ƽĳ��Ʈ ����ϱ� ���� ��� 

// ����
#define SERV_IP				"127.0.0.1"	// ���� ������
#define SERV_PORT			8282			// ���� ��Ʈ
#define FILE_ADDRESS		"DATA\\"		// ���ϵ� ����� ��ġ
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
#define CLIENT_MAX_NUM		10				// Ŭ���̾�Ʈ �ִ밹�� 10
#define NODE_MAX_NUM		100				// ��� �ִ밹�� 100
#define SENSOR_LIMIT		90				// ���� �Ѱ� 90
//---------------------- �谢�� ���� -----------------------


// ����ü ����
typedef struct double_list_queue *queue_pointer;
typedef struct queue_head *head_pointer;

//����ü ����
struct double_list_queue	{
	int				sensor;
	int				status;
	queue_pointer	left_link, right_link;
};
struct queue_head	{
	queue_pointer	right_link;					
	int				now_status;							// ���� ���¸� ����
	int				now_sensor;							// ���� ������ ����
	int				sum_sensor;							// ������ ������ �����ϱ����� ����
	int				cnt_sensor;							// ���� ī��Ʈ ����
	double			ave_sensor;							// ��� ������ �����ϴ� ����
	int				min_sensor;							// ����� �������� ���� ���� ������ �����ϴ� ����
	int				max_sensor;							// ����� �������� ���� ū ������ �����ϴ� ����
	char			ip[20];								// ������ Ŭ���̾�Ʈ ip �ּ� ������ ���� ����
	int				level[10];							// ��������ǥ�� ����� ������ count�ϱ� ���� �迭
};


// �Լ� ����
void initialize_queue( head_pointer quit_queue );			// ��� ť �ʱ�ȭ or ����� Ŭ���̾�Ʈ�� ť �ʱ�ȭ
int insert_queue( char *ip, int sensor );					// ť ����
void PrintError( char *msg );								// �����޼��� ���
void PrintIndex( void );									// �ε���ȭ�� ���
int SearchID( char *ip );									// �� ť ��ġã��
void SensorMaxMin( head_pointer temp );						// ������ �ִ�, �ּҰ� ���ؼ� ť�� ����
void SensorGraph( head_pointer queue );						// ������ ��������ǥ ���ϱ�
void PrintInfo( head_pointer queue );						// ������ ���� ���� ���
unsigned WINAPI ThreadFunc( void *arg );					// ������ �Լ�
int AllClientInfo ( void );									// ���ӵ� ��� Ŭ���̾�Ʈ�� ����
void PrintMenu(void);										// �޴� �Լ�
void DetailClientInfo();

	
// �������� ����
SOCKET sHandle;							// Ŭ���̾�Ʈ �ڵ�
SOCKADDR_IN	sAddr;						// ���ϱ���ü �����ּ�
SOCKADDR_IN	cAddr;						// ���ϱ���ü Ŭ���̾�Ʈ
char buf[BUF_MAX_SIZE];					// Ŭ���̾�Ʈ�� �ְ�ޱ����� buf�迭
char cmd;								// �޴����� ����
int type;								// Ÿ�� ����
int id;									// �������� ���� �ʿ��� ����
FILE	*fp;							// ���� ������
HANDLE	delay, menu;					// ��������, �̺�Ʈ �̿� �ڵ�

head_pointer	queue[CLIENT_MAX_NUM];
	
	


int main(void)
{
	// ���� ���̺귯�� �ʱ�ȭ
	WSADATA	wsa;
	WSAStartup( MAKEWORD(2,2), &wsa );
	
	// ���� ���� - UDP
	sHandle = socket( PF_INET, SOCK_DGRAM, 0);
	if( sHandle == INVALID_SOCKET )
		PrintError("UDP ���� ���� ����");

	// ���� �ּҼ���
	memset(&sAddr, 0, sizeof(sAddr) );
	
	sAddr.sin_family		=	AF_INET;
	sAddr.sin_addr.s_addr	=	inet_addr(SERV_IP);
	sAddr.sin_port			=	htons(SERV_PORT);

	
	bind( sHandle, (SOCKADDR *)&sAddr, sizeof(sAddr) );
	// -------------------------------- ����� ���� UDP ���� ���� �Ϸ� --------------------------------
	HANDLE	hThread;		// ������ �ڵ�	
	unsigned threadID;		// ������ID

	
	initialize_queue(NULL);	// ����Ʈ �ʱ�ȭ
	


	printf(" > Ŭ���̾�Ʈ ������ ��ٸ������Դϴ�... \n");
	
	delay	= CreateSemaphore(NULL, 1, 1, NULL);			// �������� ����
	menu	= CreateEvent(NULL, TRUE, FALSE, NULL);			// �̺�Ʈ ����

	hThread = (HANDLE)_beginthreadex( NULL, 0, ThreadFunc, 0, 0, &threadID );	// ������ ����

	while(1)	{
		WaitForSingleObject(menu, INFINITE);


		scanf("%c",&cmd);					// �޴� ����

		fflush(stdin);

		int cnt = 0;
		//--------- ��� Ŭ���̾�Ʈ ��������Ǿ����� �޴����� X-------
		for(int i=0; i<CLIENT_MAX_NUM; i++)	{
			if(queue[i]->right_link == NULL)
				cnt++;
		}
		if(cnt == CLIENT_MAX_NUM)	{
			ResetEvent(menu);
			system("cls");
			printf(" > ��� Ŭ���̾�Ʈ�� ���� ����Ǿ����ϴ�... \n");
			printf(" > Ŭ���̾�Ʈ ������ ��ٸ������Դϴ�... \n");
			continue;
		}
		//--------------------------------------------------------------

		switch(cmd)	{
			case '1':
				// Ŭ���̾�Ʈ �������� ����
				WaitForSingleObject(delay, INFINITE);
				
				type = 1;

				printf(">> Ŭ���̾�Ʈ�� �������ּ���(1 ~ %d) : ", CLIENT_MAX_NUM);
				scanf("%d",&id);

				while( id > CLIENT_MAX_NUM || id < 1 || queue[id-1]->right_link == NULL )	{
					printf("[ ERROR ] ��ȿ���� ���� Ŭ���̾�Ʈ �ѹ��Դϴ�. �ٽ��Է����ּ���. \n");
					printf(">> Ŭ���̾�Ʈ�� �������ּ���(1 ~ %d) : ", CLIENT_MAX_NUM);
					scanf("%d",&id);
				}
				id--;
			
				ReleaseSemaphore(delay, 1, NULL);
				break;

			case '2':
				// Ŭ���̾�Ʈ �������� �ݱ�
				type = 0;
				break;
	
			case 'q':
				// ���α׷� �����ϱ�
				WaitForSingleObject(hThread,INFINITE);
				PrintIndex();							// �ε���ȭ�� ���
				printf("\n\n");
				printf(" >>> ���α׷� ���� �غ� ���Դϴ�. ��ٷ��ּ���... \n");
							
				break;
		}

		if(cmd == 'q')
			break;
	}
	

	system("cls");
	printf(" >>> ���α׷� ���� ���� �Ϸ�... \n");
	getch();
	return 0;
}





// ť �ʱ�ȭ �Լ�
void initialize_queue(head_pointer quit_queue)	{

	if(buf[BUF_STATUS] == STATUS_EXIT)	{
		// ����� Ŭ���̾�Ʈ�� ���ִ� ť�� �ʱ�ȭ
		
		
		free(quit_queue);						// �޸� �ʱ�ȭ

		// ����� ---------------------------------------------------
		quit_queue = (head_pointer)malloc(sizeof(struct queue_head));
		quit_queue->right_link = NULL;
		quit_queue->now_status = 0;									// ���� ���¸� �����ϴ� ���� �ʱ�ȭ
		quit_queue->now_sensor = 0;									// ���� �������� �����ϴ� ���� �ʱ�ȭ
		quit_queue->sum_sensor = 0;									// ������ �����ϴ� ���� �ʱ�ȭ
		quit_queue->cnt_sensor = 0;									// ������ ī��Ʈ ���� �ʱ�ȭ
		quit_queue->ave_sensor = 0.0;								// ��� ������ �����ϴ� ���� �ʱ�ȭ
		quit_queue->min_sensor = 0;									// �ּ� ������ �����ϴ� ���� �ʱ�ȭ
		quit_queue->max_sensor = 0;									// �ִ� ������ �����ϴ� ���� �ʱ�ȭ
		memset(quit_queue->ip, NULL, sizeof(quit_queue->ip));		// ip�ּ� �����ϴ� char �迭 �ʱ�ȭ
		memset(quit_queue->level, 0 , sizeof(quit_queue->level));	// ����� ������ �����ϴ� int �迭 �ʱ�ȭ
	}
	else	{
		// ��� ť �ʱ�ȭ
		for(int i=0; i<CLIENT_MAX_NUM; i++)
		{
			queue[i] = (head_pointer)malloc(sizeof(struct queue_head));
			queue[i]->right_link = NULL;
			queue[i]->now_status = 0;									// ���� ���¸� �����ϴ� ���� �ʱ�ȭ
			queue[i]->now_sensor = 0;									// ���� �������� �����ϴ� ���� �ʱ�ȭ
			queue[i]->sum_sensor = 0;									// ������ �����ϴ� ���� �ʱ�ȭ
			queue[i]->cnt_sensor = 0;									// ������ ī��Ʈ ���� �ʱ�ȭ
			queue[i]->ave_sensor = 0.0;									// ��� ������ �����ϴ� ���� �ʱ�ȭ
			queue[i]->min_sensor = 0;									// �ּ� ������ �����ϴ� ���� �ʱ�ȭ
			queue[i]->max_sensor = 0;									// �ִ� ������ �����ϴ� ���� �ʱ�ȭ
			memset(queue[i]->ip, NULL, sizeof(queue[i]->ip));			// ip�ּ� �����ϴ� char �迭 �ʱ�ȭ
			memset(queue[i]->level, 0 , sizeof(queue[i]->level));		// ����� ������ �����ϴ� int �迭 �ʱ�ȭ
			
		}
	}
}
// ť ���� �Լ�
int insert_queue(char *ip, int sensor)	{
	queue_pointer new_node = (queue_pointer)malloc(sizeof(struct double_list_queue));	// �����
	if(new_node == NULL)
		PrintError("�޸� �Ҵ� ����");

	// ---------------------------- ��ť ��� �۾� ---------------------------------
	int ID = SearchID(ip);					// �� ť ã��

	if(ID == -1)	{
		// ��� ť�� ������ �Ҵ� ���޾������
		buf[BUF_STATUS] = STATUS_FULL_QUEUE;
		sendto( sHandle, buf, sizeof(buf), 0, (SOCKADDR*)&cAddr, sizeof(cAddr) );	// Ŭ���̾�Ʈ�� ����
			
		return 0;
	}


	queue[ID]->now_sensor = sensor;														// �ش�ť�� ���缾���� ������ ���� ����������
	queue[ID]->cnt_sensor++;															// �ش�ť�� ���� ī��Ʈ���� 1 ����
	queue[ID]->sum_sensor += sensor;													// �ش�ť�� ������ ���տ� ������ ����
	queue[ID]->ave_sensor = (double)queue[ID]->sum_sensor/queue[ID]->cnt_sensor;		// �ش�ť�� ��հ� ����
	SensorMaxMin( queue[ID] );															// �ش�ť�� �������� �ִ� �ּڰ� ���ؼ� queue[ID]�� ������ ����


	fseek(fp, 0, SEEK_END);	// ���� �ǳ����� �̵�
	fprintf(fp, "%d \t %d \t\t %d \t\t %d \t\t %.2lf \n",queue[ID]->cnt_sensor, queue[ID]->now_sensor, queue[ID]->max_sensor, queue[ID]->min_sensor, queue[ID]->ave_sensor);
	fclose(fp);				// ���� �Է�����
	
	// ---------------------------- ��� ��� �۾� ---------------------------------
	new_node->right_link = NULL;
	new_node->sensor = sensor;
	if( sensor > SENSOR_LIMIT || sensor == SENSOR_LIMIT )	{	// ������ >= 80 �̸�
		queue[ID]->now_status = STATUS_PAUSE;					// ť�� ������� PAUSE
		new_node->status = STATUS_PAUSE;						// ���� PAUSE( = 0 )
		buf[BUF_STATUS] = STATUS_PAUSE;							// ���� PAUSE( = 0 )
		buf[BUF_SENSOR] = queue[ID]->now_sensor;				// ���� ������ ���� ������ �־���
		sendto( sHandle, buf, sizeof(buf), 0, (SOCKADDR*)&cAddr, sizeof(cAddr) );	// Ŭ���̾�Ʈ�� ����
	}
	else	{													// ������ < 80 �̸�
		queue[ID]->now_status = STATUS_RUN;						// ť�� ������� RUN
		new_node->status = STATUS_RUN;							// ���� RUN ( = 1 )
		buf[BUF_STATUS] = STATUS_RUN;							// ���� PAUSE( = 0 )
		buf[BUF_SENSOR] = queue[ID]->now_sensor;				// ���� ������ ���� ������ �־���
		sendto( sHandle, buf, sizeof(buf), 0, (SOCKADDR*)&cAddr, sizeof(cAddr) );	// Ŭ���̾�Ʈ�� ����
	}
	// -----------------------------------------------------------------------------
	


	if( queue[ID]->right_link == NULL )	{
		// ù ����϶��� ť ����
		queue[ID]->right_link = new_node;
	}
	else	{	
		// ù��尡 �ƴҶ��� ť ����

		// --------------------------- ��� �� ��� ���� -------------------------------
		if( queue[ID]->cnt_sensor % NODE_MAX_NUM == 1 )	{
			// ����ī��Ʈ�� NODE_MAX_NUM���� ���������� 1�̸�, ������� NODE_MAX_NUM�� 100�϶� 101��° ����̸� �� �� ��� ����
			// (ù���� �Ű澵�ʿ� ����. �� �κп� �������� �����Ƿ�.)
			queue_pointer	delete_temp;			// �� �� ��� ������ delete_temp
			delete_temp = queue[ID]->right_link;
			
			queue[ID]->right_link = delete_temp->right_link;
			queue[ID]->right_link->left_link = NULL;
			free(delete_temp);
		}
		// --------------------------- ��� �� ��忡 �߰� -----------------------------
		queue_pointer move_temp;					// �� �� ���� �̵��� move_temp
		move_temp = queue[ID]->right_link;

		while( move_temp->right_link != NULL )	{
			//�� ������ �̵�
			move_temp = move_temp->right_link;
		}
		move_temp->right_link = new_node;			// ����� �߰�
		new_node->left_link = move_temp;			// ������ ������ ��� ����
		//------------------------------------------------------------------------------
	}

	WaitForSingleObject(delay,INFINITE);
	// --------------------- ��ť�� ���� �Ϸ��� �������� �۾� ----------------------
	SensorGraph( queue[ID] );				// �ش�ť�� �������� ������ Ȯ�� �Ҽ� �ִ� ǥ ����

	PrintIndex();							// �ε���ȭ�� ���
	PrintInfo( queue[ID] );					// �������� ���� ������ ���
	AllClientInfo();						// ��� Ŭ���̾�Ʈ ���� ���

	if(type == 1)							// Ŭ���̾�Ʈ �ڼ������� ����ʹٴ� ��û�� �޾�����
		DetailClientInfo();					// �Լ�ȣ��
	
	PrintMenu();							// �޴� ���
	
	SetEvent(menu);						// �޴� ����� �޴� �Է°���
	//------------------------------------------------------------------------------
	ReleaseSemaphore(delay, 1, NULL);

	return 0;
}

// �ε���ȭ�� ��� �Լ�
void PrintIndex(void)	{
	system("cls");
	printf("������������������������������������������������������������������������������\n");
	printf("��                                                                          ��\n");
	printf("��                                                                          ��\n");
	printf("��                 Mornitoring                                              ��\n");
	printf("��                                        System  (Server)                  ��\n");
	printf("��                                                                          ��\n");
	printf("��                                                                          ��\n");
	printf("������������������������������������������������������������������������������\n\n");
}
// �� ťã�� �Լ�
int SearchID(char *ip)	{
	int id = -1 ;

	char file_name[50];
	
	for(int i=0; i<CLIENT_MAX_NUM; i++)	{
		if( !strcmp(queue[i]->ip,ip) )	{
			// ������ �����ǰ� �ִ��� �˻�
			sprintf(file_name, "%sCLIENT %s.txt", FILE_ADDRESS ,ip);
			//sprintf(file_name, "CLIENT %s.txt", ip);

			fp = fopen(file_name, "r+");

			id = i;		// id�� i�� ����
			break;
		}
	}

	if( id == -1 )	{
		// �˻��� ������ �������ּҰ� ������ ���� �߰�
		for(int i=0; i<CLIENT_MAX_NUM; i++)	{
			if( *queue[i]->ip == NULL  )	{
				strncpy(queue[i]->ip, ip,strlen(ip));		// queue->ip�� ����
				
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

// �����޼��� ��� �Լ�
void PrintError(char *msg)	{
	printf("> [ERROR] %s \n",msg);
	exit(0);
}
// ������ �ִ�, �ּҰ� ����� ����
void SensorMaxMin( head_pointer queue  )	{
		
	if( queue->cnt_sensor == 1 )	{
		// ��尡 �Ѱ��ۿ�������
		queue->max_sensor = queue->now_sensor;		// �� ť�� �ִ񰪿� ������ ����
		queue->min_sensor = queue->now_sensor;		// �� ť�� �ּڰ��� ������ ����
 	}

	else	{
		// ��尡 �� �� �̻��϶�
		if(queue->max_sensor < queue->now_sensor )		// �� ť�� �ִ��� ��� ���� ���������� ������ ���� 
			queue->max_sensor = queue->now_sensor;
		else
			queue->max_sensor;
		
		if(queue->min_sensor > queue->now_sensor )		// �� ť�� �ּڰ��� ��� ���� ���������� ũ�� ����
			queue->min_sensor = queue->now_sensor;
		else
			queue->min_sensor;
	}
}

// �ش� ť�� �������� ������ Ȯ�� �Ҽ� �ִ� ǥ ����
void SensorGraph(head_pointer queue)
{
	queue_pointer last_node;
	
	int temp;
	
	last_node = queue->right_link;

	temp = queue->now_sensor / 10;	// ������ ��� ���
	queue->level[temp]++;			// �ش� ����� ������ 1�� ����
	
}

// �ش� ť�� ���� & ��� ť�� ���� & �޴��� �����Լ�
void PrintInfo(head_pointer enter_queue)	{
	if(enter_queue->now_status != STATUS_EXIT)	{
		// ����Ǽ� ȣ��Ȱ� �ƴҶ�
		printf("\t   ��������������������������������������������������������\n");
		printf("\t   ��\t\tLately\t\t\t\t\t ��\n");
		printf("\t   ��\t\t\tClient [ %s ]\t ��\n", enter_queue->ip);
		printf("\t   ��\t\t\t\t\t\t\t ��\n");
		printf("\t   ��\t\t ������ : %d \t\t\t\t ��\n", enter_queue->now_sensor);
		printf("\t   ��\t\t �ִ� : %d \t\t\t\t ��\n", enter_queue->max_sensor);
		printf("\t   ��\t\t �ּڰ� : %d \t\t\t\t ��\n", enter_queue->min_sensor);
		printf("\t   ��\t\t ��հ� : %.2lf \t\t\t ��\n", enter_queue->ave_sensor);
		printf("\t   ��������������������������������������������������������\n");
	}
	else	{
		// ����Ǽ� ȣ��Ǿ�����
		printf("\t   ��������������������������������������������������������\n");
		printf("\t   ��\t\tLately\t\t\t\t\t ��\n");
		printf("\t   ��\t\t\tClient [ %s ]\t ��\n", enter_queue->ip);
		printf("\t   ��\t\t\t\t\t\t\t ��\n");
		printf("\t   ��\t\t\t\t\t\t\t ��\n");
		printf("\t   ��\t\t     ��   ��   ��   ��\t\t\t ��\n");
		printf("\t   ��\t\t\t\t\t\t\t ��\n");
		printf("\t   ��\t\t\t\t\t\t\t ��\n");
		printf("\t   ��������������������������������������������������������\n");
	}

}
// ��ü ���� ��� �Լ�
int AllClientInfo(void)	{
	printf("\n\n");
	printf(" ������������������������[ ��� ���� Ŭ���̾�Ʈ���� ]������������������������ \n");
	printf(" NUM \t IP \t\t LATELY_SENSOR \t AVERAGE_SENSOR \t STATUS \n");
	for(int i=0; i<CLIENT_MAX_NUM; i++)	{
	
		if( queue[i]->right_link != NULL )	{
			// ù��尡 ����Ǿ������,�� Ŭ���̾�Ʈ�κ��� �ϳ��� �������� �޾������
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
	printf(" ���������������������������������������������������������������������������� \n\n");
		



	return 0;
}
// �޴� �Լ�
void PrintMenu(void)	{

	printf("\n");
	printf("[ �� �� ] ������������������������������������������������������������������ \n");
	printf("  1) Ŭ���̾�Ʈ ���� �ڼ��� ���� \n");
	printf("  2) Ŭ���̾�Ʈ ���� �ڼ��� ���� �ݱ� \n");
	printf("  q) �����ϱ� \n\n");
	printf("> �����ϼ��� :  ");
	
}
// ������ Ŭ���̾�Ʈ ���� �ڼ������� �Լ�
void DetailClientInfo()	{
	int cnt=0;

	if(queue[id]->now_status == STATUS_EXIT)	{
		printf("\t   ��������������������������������������������������������\n");
		printf("\t   ��\t\tSelected\t\t\t\t ��\n");
		printf("\t   ��\t\t\tClient [ %s ]\t ��\n", queue[id]->ip);
		printf("\t   ��\t\t\t\t\t\t\t ��\n");
		printf("\t   ��\t\t\t\t\t\t\t ��\n");
		printf("\t   ��\t\t     ��   ��   ��   ��\t\t\t ��\n");
		printf("\t   ��\t\t\t\t\t\t\t ��\n");
		printf("\t   ��\t\t\t\t\t\t\t ��\n");
		printf("\t   ��������������������������������������������������������\n");
		type=0;
	}
	else	{
		printf("\t   ��������������������������������������������������������\n");
		printf("\t   ��\t\tSelected\t\t\t\t ��\n");
		printf("\t   ��\t\t\tClient [ %s ]\t ��\n", queue[id]->ip);
		printf("\t   ��\t\t\t\t\t\t\t ��\n");
		printf("\t   ��\t\t ������ : %d \t\t\t\t ��\n", queue[id]->now_sensor);
		printf("\t   ��\t\t �ִ� : %d \t\t\t\t ��\n", queue[id]->max_sensor);
		printf("\t   ��\t\t �ּڰ� : %d \t\t\t\t ��\n",	queue[id]->min_sensor);
		printf("\t   ��\t\t ��հ� : %.2lf \t\t\t ��\n", queue[id]->ave_sensor);
		printf("\t   ��������������������������������������������������������\n");

		printf("\n\t       ������������������[ ���� ����ǥ ]������������������ \n");
		for(int i=0; i < 10; i++)	{
			// ��������ǥ ���
			printf("\t\t\t%d ~ %d\t��", cnt, cnt+9);

			for(int j=0; j<queue[id]->level[i]; j++){
				printf("��");
			}
			cnt += 10;
			printf("\n");

		}
		printf("\t      ����������������������������������������������������\n\n");
	}
}

// ������ �Լ�
unsigned WINAPI ThreadFunc(void *arg)	{
	int cSize;
	int ID;
	cSize = sizeof(cAddr);

	while(cmd != 'q' )	{
		recvfrom( sHandle, buf , sizeof(buf), 0, (SOCKADDR *)&cAddr, &cSize);

		if(buf[BUF_STATUS] == STATUS_EXIT)	{
			// Ŭ���̾�Ʈ�κ��� ���� BUF_STATUS�� 2, �� ����ó���̸�
			ID= SearchID( inet_ntoa(cAddr.sin_addr) );
			queue[ID]->now_status = STATUS_EXIT;

			WaitForSingleObject(delay,INFINITE);
			//-----------------------------------------------------------------------------
			PrintIndex();							// �ε���ȭ�� ���
			PrintInfo( queue[ID] );					// ť ���� ���
			AllClientInfo();						// ��� Ŭ���̾�Ʈ ���� ���
			if(type == 1)							// Ŭ���̾�Ʈ �ڼ������� ����ʹٴ� ��û�� �޾�����
				DetailClientInfo();					// �Լ�ȣ��
			PrintMenu();							// �޴� ���
			//-----------------------------------------------------------------------------
			ReleaseSemaphore(delay, 1, NULL);
		
			initialize_queue( queue[ID] );			// ť �ʱ�ȭ ����

			int cnt=0;
			for(int i=0; i<CLIENT_MAX_NUM; i++)	{
				if(queue[i]->right_link == NULL)
					cnt++;
			}
			if(cnt == CLIENT_MAX_NUM)	{
				Sleep(2 * 1000);
				system("cls");
				printf(" > ��� Ŭ���̾�Ʈ�� ���� ����Ǿ����ϴ�... \n");
				printf(" > Ŭ���̾�Ʈ ������ ��ٸ������Դϴ�... \n");
			}
				
		}
		else
			// Ŭ���̾�Ʈ�κ��� ���� BUF_STATUS�� 2���ƴѰ��, �� �����̸�
			insert_queue( inet_ntoa(cAddr.sin_addr), buf[BUF_SENSOR] );
	}

	while(cmd == 'q')	{
		// ����޴��� ���ù޾�����
		recvfrom( sHandle, buf , sizeof(buf), 0, (SOCKADDR *)&cAddr, &cSize);
		
		ID= SearchID( inet_ntoa(cAddr.sin_addr) );

		buf[BUF_STATUS] = STATUS_EXIT;							// ���� EXIT( = 2 )
		queue[ID]->now_status = STATUS_EXIT;

		initialize_queue( queue[ID] );							// ť �ʱ�ȭ ����

			
		sendto( sHandle, (char *)&buf[BUF_STATUS], sizeof(buf[BUF_STATUS]), 0, (SOCKADDR*)&cAddr, sizeof(cAddr) );	// ���� ����Ǿ��ٴ� �޼��� ������

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
