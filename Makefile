NOME_PROJ = median
CC = gcc
FLAGS = -g -pg
LIBS = -lpng -lm -lOpenCL

SRC = $(wildcard *.c)   # Find all .c source files
OBJ = $(SRC:.c=.o)      # Generate object files from each .c file

# Default target
all: $(NOME_PROJ)

# Linking the object files to create the executable
$(NOME_PROJ): $(OBJ)
	$(CC) $(FLAGS) -o $(NOME_PROJ) $(OBJ) $(LIBS)

# Compiling each .c file into .o object files
%.o: %.c
	$(CC) $(FLAGS) -c $< -o $@

# Clean up object files and the binary
clean:
	rm -f $(OBJ) $(NOME_PROJ)

# Run the program with input and output arguments
run: all
	./$(NOME_PROJ) imagem_com_ruido.png output.png  

# Benchmark the execution with hyperfine
bench: all
	hyperfine --warmup 3 "./$(NOME_PROJ) imagem_com_ruido.png output.png"

gprof: all
	./$(NOME_PROJ) imagem_com_ruido.png output.png
	gprof $(NOME_PROJ) gmon.out > analysis.txt