EXE = VectorAddProf
SRC = VectorAddProf.c
OBJ = VectorAddProf.o

CLROOT = /opt/AMDAPP

CFLAG = -std=c99 -Wall -g
LDFLAG = 
INC = -I$(CLROOT)/include
LIB = -L$(CLROOT)/lib/x86_64 -lOpenCL

all: $(EXE)

$(EXE): $(OBJ)
	gcc -o $@ $(LDFLAG) $^ $(LIB)

$(OBJ): $(SRC)
	gcc -o $@ $(CFLAG) $(INC) -c $<

clean:
	rm -fr $(EXE) $(OBJ)

