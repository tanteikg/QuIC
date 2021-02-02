CFLAGS = -g

target : QuIC.exe QuICrun.exe

QuIC.exe : visualizer.c q_emul.h q_emul.o q_oracle.h q_oracle.o
	gcc $(CFLAGS) visualizer.c q_emul.o q_oracle.o -o QuIC.exe

QuICrun.exe : QuICrun.c q_emul.h q_emul.o q_oracle.h q_oracle.o
	gcc $(CFLAGS) QuICrun.c q_emul.o q_oracle.o -o QuICrun.exe

q_emul.o : q_emul.c q_emul.h q_oracle.h
	gcc $(CFLAGS) -c q_emul.c -o q_emul.o

q_oracle.o : q_oracle.c q_oracle.h q_emul.h
	gcc $(CFLAGS) -c q_oracle.c -o q_oracle.o

clean :
	rm -f QuIC.exe QuICrun.exe q_emul.o q_oracle.o *.exe.stackdump
