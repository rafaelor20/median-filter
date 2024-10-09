NOME_PROJ=median
CC=g++
FLAGS= -g -pg -lpng

SRC=$(wildcard *.cpp)   # Assuming your source files are .cpp
OBJ=$(SRC:.cpp=.o)      # Generate object files for each .cpp

all: $(NOME_PROJ)

$(NOME_PROJ): $(OBJ)
	$(CC) $(FLAGS) -o $(NOME_PROJ) $(OBJ)

%.o: %.cpp
	$(CC) $(FLAGS) -c $< -o $@

clean:
	rm -f $(OBJ) $(NOME_PROJ)

# Run the program with input and output arguments
run: all
	./$(NOME_PROJ) imagem_com_ruido.png output.png  

# Benchmark using hyperfine
bench: all
	hyperfine --warmup 3 "./$(NOME_PROJ) imagem_com_ruido.png output.png" 