CLROOT = /opt/AMDAPP

CXX = g++
INC = -I$(CLROOT)/include
LIB = -L$(CLROOT)/lib/x86_64 -lOpenCL -lrt

EXE = BufferBandwidth
SRC = $(wildcard *.cpp)
OBJDIR = obj
OBJ = $(patsubst %.cpp, $(OBJDIR)/%.o, $(SRC))

all: makeobjdir $(EXE)

makeobjdir:
	mkdir -p $(OBJDIR)

$(EXE): $(OBJ)
	$(CXX) -o $@ $^ $(LIB)

$(OBJDIR)/%.o: %.cpp
	$(CXX) -o $@ $(INC) -c $<

.PHONY: clean
clean:
	$(RM) -r $(EXE) $(OBJDIR)

