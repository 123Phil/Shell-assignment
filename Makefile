all: proc_parse shell

proc_parse:	proc_parse.c
	gcc proc_parse.c -o proc_parse

shell:	phil_shell.c
	gcc phil_shell.c -o shell

clean:
	rm -rf proc_parse shell
