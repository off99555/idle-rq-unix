all:
	gcc server.c -o server
	gcc idle-rq.c trouble-maker.c client.c -o client

clean:
	$(RM) server client result_client.txt result_server.txt
