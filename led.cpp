#include "led.h"

led::led() :
	working(true),
	enable(true),
	frequency(2),
	color(BLUE),
	on(true),
	rfifo_fn("/var/tmp/led_r.fifo"),
	wfifo_fn("/var/tmp/led_w.fifo")
{
	color_names.insert(std::pair<ledcolor, std::string>(RED, "red"));
	color_names.insert(std::pair<ledcolor, std::string>(GREEN, "green"));
	color_names.insert(std::pair<ledcolor, std::string>(BLUE, "blue"));

	remove(rfifo_fn.c_str());
	remove(wfifo_fn.c_str());
	if(mkfifo(rfifo_fn.c_str(), 0777) < 0)
	{
		std::cout << strerror(errno) << std::endl;
		exit(1);
	}
	if((rfd = open(rfifo_fn.c_str(), O_RDONLY | O_NONBLOCK)) < 0)
	{
		std::cout << strerror(errno) << std::endl;
		remove(rfifo_fn.c_str());
		exit(1);
	}
	if(mkfifo(wfifo_fn.c_str(), 0777) < 0)
	{
		std::cout << strerror(errno) << std::endl;
		exit(1);
	}

	pthread_create(&work_th, NULL, &work_loop, (void *) this);
}

void led::outstate(led *ledc)
{
	while(system("clear") & 0);
	std::cout << "\e[40m\e[0;" << 
		(!ledc->on ? "30m[" :
		(ledc->color == RED ? "31m[" :
		(ledc->color == GREEN ? "32m[" :
		"34m["))) << 
		ledc->color_names[ledc->color] << "]\e[00m" << std::endl;
}

void *led::work_loop(void *cptr)
{
	led *ledc = (led *) cptr;
	for(;ledc->working;)
	{
			outstate(ledc);
			if(ledc->enable && ledc->frequency)
			{
				ledc->on = !ledc->on;
				usleep(1000000 / ledc->frequency);
			}
			else
			{
				ledc->on = ledc->enable;
				usleep(100000);
			}
	}
	return NULL;
}

void led::answer(const char *data)
{
	if((wfd = open(wfifo_fn.c_str(), O_WRONLY)) > 0)
	{
		write(wfd, data, strlen(data));
		close(wfd);
	}
}

int led::parse(const char *buf, ledcmdinfo &info)
{
        std::string rstr(buf);

        size_t start_pos = rstr.find(' ', 0);
        if(start_pos == std::string::npos && info.arg_type != LED_TYPE_NONE ||
           rstr.at(rstr.size() - 1) != '\n')
                return -1;

	if(rstr.find("set-led-state") != std::string::npos)
	{
		info.cmd = SET_LED_STATE;
		info.arg_type = LED_TYPE_BOOL;
	}
	else if(rstr.find("get-led-state") != std::string::npos)
	{
		info.cmd = GET_LED_STATE;
		info.arg_type = LED_TYPE_NONE;
	}
	else if(rstr.find("set-led-rate") != std::string::npos)
	{
		info.cmd = SET_LED_RATE;
		info.arg_type = LED_TYPE_INT1D;
	}
	else if(rstr.find("get-led-rate") != std::string::npos)
	{
		info.cmd = GET_LED_RATE;
		info.arg_type = LED_TYPE_NONE;
	}
	else if(rstr.find("set-led-color") != std::string::npos)
	{
		info.cmd = SET_LED_COLOR;
		info.arg_type = LED_TYPE_COLOR;
	}
	else if(rstr.find("get-led-color") != std::string::npos)
	{
		info.cmd = GET_LED_COLOR;
		info.arg_type = LED_TYPE_NONE;
	}
	else
		return -1;

	size_t pos;
	switch(info.arg_type)
	{
	case LED_TYPE_INT1D:
		if((pos = rstr.find_first_of("0123456789", start_pos)) == std::string::npos)
			return -1;
		else
		{
			info.val_int = rstr[pos] - 48;
			if(info.val_int < 0 ||
			   info.val_int > 9)
				return -1;
		}

		break;
	case LED_TYPE_BOOL:
		if(rstr.find("on", start_pos) != std::string::npos)
			info.val_bool = true;
		else if(rstr.find("off", start_pos) != std::string::npos)
			info.val_bool = false;
		else
			return -1;
		break;
	case LED_TYPE_COLOR:
		if(rstr.find("red", start_pos) != std::string::npos)
			info.val_color = RED;
		else if(rstr.find("green", start_pos) != std::string::npos)
			info.val_color = GREEN;
		else if(rstr.find("blue", start_pos) != std::string::npos)
			info.val_color = BLUE;
		else
			return -1;
		break;
	case LED_TYPE_NONE:
		return 0;
	default:
		return -1;
	}

	return 0;
}

void led::ipc_loop()
{
	char rbuf[rbuf_size];
	for(int r = 0; r >= 0; r = read(rfd, rbuf, rbuf_size))
	{
		if(r > 0)
		{
			ledcmdinfo info;
			std::stringstream ans_stm;
			if(parse(rbuf, info) < 0)
				answer("FAILED\n");
			else
			{
				switch(info.cmd)
				{
				case SET_LED_STATE:
					if(info.val_bool ^ enable)
					{
						enable = info.val_bool;
						ans_stm << "OK " << (enable ? "on" : "off") << std::endl;
						answer(ans_stm.str().c_str());
					}
					else
						answer("FAILED\n");
					break;
				case GET_LED_STATE:
					ans_stm << "OK " << (enable ? "on" : "off") << std::endl;
					answer(ans_stm.str().c_str());
					break;
				case SET_LED_RATE:
					if(info.val_int <= 5)
					{
						frequency = info.val_int;
						ans_stm << "OK " << frequency << std::endl;
						answer(ans_stm.str().c_str());
					}
					else
						answer("FAILED\n");
					break;
				case GET_LED_RATE:
					ans_stm << "OK " << frequency << std::endl;
					answer(ans_stm.str().c_str());
					break;
				case SET_LED_COLOR:
					color = info.val_color;
					ans_stm << "OK " << color_names[color] << std::endl;
					answer(ans_stm.str().c_str());
				case GET_LED_COLOR:
					ans_stm << "OK " << color_names[color] << std::endl;
					answer(ans_stm.str().c_str());
					break;
				default:
					answer("FAILED\n");
					break;
				};
			}
		}
		usleep(10000);
		bzero(rbuf, rbuf_size);
	}

}

int main(int argc, char *argv[])
{
	led led;
	led.ipc_loop();
	return 0;
}
