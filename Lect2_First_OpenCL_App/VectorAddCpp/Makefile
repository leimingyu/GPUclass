EXE = VectorAdd
SRC = VectorAdd.cpp
OBJ = VectorAdd.o

CLROOT = /opt/AMDAPP

CFLAG = -std=c++0x -Wall -g
LDFLAG = 
INC = -I$(CLROOT)/include
LIB = -L$(CLROOT)/lib/x86_64 -lOpenCL

all: $(EXE)

$(EXE): $(OBJ)
	g++ -o $@ $(LDFLAG) $^ $(LIB)

$(OBJ): $(SRC)
	g++ -o $@ $(CFLAG) $(INC) -c $<

clean:
	rm -fr $(EXE) $(OBJ)

