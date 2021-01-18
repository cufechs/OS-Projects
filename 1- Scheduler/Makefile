build:
	gcc process_generator.c -o process_generator.out -g
	gcc clk.c -o clk.out -g
	gcc scheduler.c -o scheduler.out -g
	gcc process.c -o process.out -g
	gcc test_generator.c -o test_generator.out -g

clean:
	rm -f *.out  processes.txt

all: clean build

run:
	clear
	./process_generator.out
