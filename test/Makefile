
SRC = $(shell find ../src/*.cpp *.cpp)
OBJ = ${SRC:.cpp=.o}
DEPS = $(OBJ:.o=.d)
INC = ../include #$(shell find include/)

INC_FLAGS := $(addprefix -I,$(INC))

CC = g++

OPTIMIZATION_FLAG = 
WARNING_FLAG = -Wall -Wextra -Werror -Wno-error=pointer-arith
DEBUG_FLAG = -g

CFLAGS = -c 
LDFLAGS += -lm -Wl,--gc-sections -flto
CFLAGS += $(OPTIMIZATION_FLAG)
CFLAGS += $(WARNING_FLAG)
CFLAGS += $(DEBUG_FLAG)
CFLAGS += $(INC_FLAGS)

CFLAGS += $(LIB_CFLAGS)
LDFLAGS += $(LIB_LDFLAGS)


%.o:%.cpp
	@$(CC) -c -o $@ $< $(CFLAGS)

OUT = test
all: test
$(OUT): $(OBJ)
	@$(CC) -o $@ $^ $(LDFLAGS) 

#valgrind:
#	make clean && make && valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes ./fbgc

clean:
	@rm -f $(OBJ)
