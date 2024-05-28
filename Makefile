Target := iter_tcp_srv_arith

$(Target): iter_tcp_srv_arith.c
	gcc -o $@ $^
cli:iter_tcp_cli_arith.c
	gcc -o iter_tcp_cli_arith iter_tcp_cli_arith.c