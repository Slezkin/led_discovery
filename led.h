#ifndef LED_CLIENT_H
#define LED_CLIENT_H

#include <iostream>
#include <string>
#include <sstream>
#include <map>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>
#include <errno.h>
#include <pthread.h>

class led
{
	typedef enum {RED, GREEN, BLUE} ledcolor;
public:
	bool working;
	bool enable;
	int frequency;
	ledcolor color;
	bool on;


	led();
	void ipc_loop();
private:
	static const int rbuf_size = 1000;
	typedef enum
	{
		SET_LED_STATE,
		GET_LED_STATE,
		SET_LED_COLOR,
		GET_LED_COLOR,
		SET_LED_RATE,
		GET_LED_RATE,
	} ledcmd;
	typedef enum
	{
		LED_TYPE_NONE,
		LED_TYPE_INT1D,
		LED_TYPE_BOOL,
		LED_TYPE_COLOR,
	} ledatype;
	typedef struct
	{
		ledcmd cmd;
		ledatype arg_type;
		union
		{
			int val_int;
			bool val_bool;
			ledcolor val_color;
		};
	} ledcmdinfo;

	std::map<ledcolor, std::string> color_names;
	int rfd, wfd;
	pthread_t work_th;
	std::string rfifo_fn, wfifo_fn;


	int parse(const char *buf, ledcmdinfo &info);
	void answer(const char *data);
	static void outstate(led *ledc);
	static void *work_loop(void *cptr);
};

#endif // LED_CLIENT_H
