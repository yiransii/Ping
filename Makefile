# create new ping
ping: ping.c
	gcc -Wall ping.c -o ping -pthread
# remove old ping
clean:
	rm -rf ping

