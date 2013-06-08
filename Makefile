all:
	gcc -Wall -lnfc -o poll_n_send poll_client.c
	gcc -Wall -o receive_n_do poll_server.c


clean:
	rm -f *~ *.o poll_n_send receive_n_do
