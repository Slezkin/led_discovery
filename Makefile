all:
	g++ -o led_server led.cpp -lpthread
clean:
	rm led_server 