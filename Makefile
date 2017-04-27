all:
	gcc -pthread idle-rq.c trouble-maker.c server.c -o server
	gcc -pthread idle-rq.c trouble-maker.c client.c -o client

clean:
	$(RM) server client result_client.txt result_server.txt
