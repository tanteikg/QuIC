
target = QuIC.exe

QuIC.exe : visualizer.c q_emul.h q_emul.o
	gcc -g visualizer.c q_emul.o -o QuIC.exe

q_emul.o : q_emul.c q_emul.h
	gcc -g -c q_emul.c -o q_emul.o

clean :
	rm -f visualizer.exe q_emul.o
