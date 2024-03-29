CFLAGS = -O

target : QuIC.exe QuICrun.exe QuICimage.exe

QuICimage.exe : QuICimage.c q_emul.h gifenc.c gifenc.h q_emul.o q_oracle.h q_oracle.o
	gcc $(CFLAGS)  -fopenmp QuICimage.c gifenc.c q_emul.o q_oracle.o -o QuICimage.exe -lm 

QuIC.exe : visualizer.c q_emul.h q_emul.o q_oracle.h q_oracle.o
	gcc $(CFLAGS) -fopenmp visualizer.c q_emul.o q_oracle.o -o QuIC.exe -lm 

QuICrun.exe : QuICrun.c q_emul.h q_emul.o q_oracle.h q_oracle.o
	gcc $(CFLAGS) -fopenmp QuICrun.c q_emul.o q_oracle.o -o QuICrun.exe -lm

q_emul.o : q_emul.c q_emul.h q_oracle.h
	gcc $(CFLAGS)  -fopenmp -c q_emul.c -o q_emul.o 

q_oracle.o : q_oracle.c q_oracle.h q_emul.h
	gcc $(CFLAGS) -c q_oracle.c -o q_oracle.o

clean :
	rm -f QuIC.exe QuICrun.exe QuICimage.exe q_emul.o q_oracle.o *.exe.stackdump

git:
	git add .
	git commit -m $(m)
	git push
