#include <sys/types.h>          /* See NOTES */
#include <sys/socket.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdio.h>
#include <signal.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>

/* socket
 * bind
 * listen
 * accept
 * send/recv
 */

#define SERVER_PORT 8888
#define BACKLOG     10
/* 请求的类型 */
#define CMD_DIRECTION		0
#define CMD_BEEP			1
#define CMD_SPEED			2
#define CMD_TEMPERATURE	3

struct masg{
	unsigned char type;
	unsigned char direction;
	unsigned char speed;		/* 0~100 */
	float temperature;
};

int fd;
int temp_fd;

void* DutyCycleThread(void *data)
{
	int duty_cycle;			// 取值范围0~100
	char val;

	while(1)
	{
		duty_cycle = *(int *)data;

		val = 1;
		write(fd, &val, 1);

		usleep(duty_cycle*1000);

		val = 0;
		write(fd, &val, 1);
		usleep(100*1000-duty_cycle*1000);
	}

	return data;
}

float get_temperature(void)
{
	float t;
	unsigned int tmp = 0;

	read(temp_fd, &tmp, sizeof(tmp));
	t = tmp * 0.0625;
	printf("tmp = %d,the current temperature is %f\n", tmp, t);

	return t;
}

int main(int argc, char **argv)
{
	int iSocketServer;
	int iSocketClient;
	struct sockaddr_in tSocketServerAddr;
	struct sockaddr_in tSocketClientAddr;
	int iRet;
	socklen_t iAddrLen;
	pthread_t Duty_Cycle_ID;
	int duty_cycle;			// 取值范围0~100

	int iRecvLen;

	int iClientNum = -1;
	struct masg request;
	int iSendLen;

	duty_cycle = 100;

	fd = open("/dev/motor", O_RDWR | O_NONBLOCK);
	if (fd < 0)
	{
		printf("can't open!\n");
		return -1;
	}

	temp_fd = open("/dev/ds18b20", O_RDWR | O_NONBLOCK);
	if (temp_fd < 0)
	{
		printf("can't open!\n");
		return -1;
	}

	signal(SIGCHLD,SIG_IGN);
	
	iSocketServer = socket(AF_INET, SOCK_STREAM, 0);
	if (-1 == iSocketServer)
	{
		printf("socket error!\n");
		return -1;
	}

	tSocketServerAddr.sin_family      = AF_INET;
	tSocketServerAddr.sin_port        = htons(SERVER_PORT);  /* host to net, short */
 	tSocketServerAddr.sin_addr.s_addr = INADDR_ANY;
	memset(tSocketServerAddr.sin_zero, 0, 8);
	
	iRet = bind(iSocketServer, (const struct sockaddr *)&tSocketServerAddr, sizeof(struct sockaddr));
	if (-1 == iRet)
	{
		printf("bind error!\n");
		return -1;
	}

	iRet = listen(iSocketServer, BACKLOG);
	if (-1 == iRet)
	{
		printf("listen error!\n");
		return -1;
	}

	while (1)
	{
		iAddrLen = sizeof(struct sockaddr);
		iSocketClient = accept(iSocketServer, (struct sockaddr *)&tSocketClientAddr, &iAddrLen);
		if (-1 != iSocketClient)
		{
			iClientNum++;
			printf("Get connect from client %d : %s\n",  iClientNum, inet_ntoa(tSocketClientAddr.sin_addr));
			if (!fork())
			{
				pthread_create(&Duty_Cycle_ID, NULL, &DutyCycleThread, &duty_cycle);
			
				/* 子进程的源码 */
				while (1)
				{
					/* 接收客户端发来的数据并显示出来 */
					iRecvLen = recv(iSocketClient, &request, sizeof(struct masg), 0);
					if (iRecvLen <= 0)
					{
						close(iSocketClient);
						return -1;
					}
					else
					{
						switch(request.type)
						{
							case CMD_DIRECTION:
								ioctl(fd, request.direction);
								//printf("Get Msg From Client %d:CMD_DIRECTION: %d\n", iClientNum, request.direction);
								break;
							case CMD_BEEP:
								ioctl(fd, request.direction);
								//printf("Get Msg From Client %d:CMD_BEEP: %d\n", iClientNum, request.direction);
								break;
							case CMD_SPEED:
								duty_cycle = request.speed;
								//printf("Get Msg From Client %d:CMD_SPEED: %d\n", iClientNum, request.speed);
								break;
							case CMD_TEMPERATURE:
								request.temperature = get_temperature();
								//printf("Get Msg From Client %d:CMD_TEMPERATURE\n", iClientNum);
								/* 应该将温度返回给客服端 */
								iSendLen = send(iSocketClient, &request, sizeof(struct masg), 0);
								if (iSendLen <= 0)
								{
									close(iSocketClient);
									return -1;
								}
								break;
							default:
								printf("unkown command\n");
								break;
						}
					}
				}				
			}
		}
	}
	
	close(iSocketServer);
	return 0;
}


