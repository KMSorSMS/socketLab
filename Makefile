server := iter_tcp_srv_arith
cli := iter_tcp_cli_arith
all:$(server) $(cli)

$(server): iter_tcp_srv_arith.c
	gcc -g -o $@ $^
$(cli): iter_tcp_cli_arith.c
	gcc -g -o $@ $^
clean:
	rm -rf $(server) $(cli)