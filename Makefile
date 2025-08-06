rbt: rbt.c
	gcc -o $@ -g -Wall -Werror -fsanitize=address $<
