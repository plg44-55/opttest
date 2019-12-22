CC = gcc-4.8
CXX = g++-4.8

IDLS =  included_data main_data
IDL_CLT_SOURCES = $(addsuffix .cpp, ${IDLS}) $(addsuffix .h, ${IDLS})
IDL_SRV_SOURCES = ${IDL_CLT_SOURCES} $(addsuffix _skel.cpp, ${IDLS}) $(addsuffix _skel.h, ${IDLS})
IDL_SOURCES = ${IDL_SRV_SOURCES}

TARGETS = server client

SRV_SOU = server.cpp ${IDL_SRV_SOURCES}
CLI_SOU = client.cpp ${IDL_CLT_SOURCES}

SRV_OBJ = $(patsubst %.h,,${SRV_SOU:.cpp=.o})
CLI_OBJ = $(patsubst %.h,,${CLI_SOU:.cpp=.o})

CPPFLAGS = -I/home/pete/tests/OB1/include -I.
CXXFLAGS = -fpermissive -ggdb -std=gnu++11 -Wno-write-strings

LDFLAGS = -L/home/pete/tests/OB1/lib
LDLIBS = -lOB -lJTC -lpthread -ldl -lstdc++
#LMALLOC = -ltcmalloc

all: ${TARGETS}

server: ${SRV_OBJ}
#	$(CC) $(LDFLAGS) $(SRV_OBJ) $(LOADLIBES) $(LDLIBS) $(LMALLOC) -o $@

client: ${CLI_OBJ}

client.cpp: $(addsuffix .h, ${IDLS})

server.cpp: $(addsuffix .h, ${IDLS}) $(addsuffix _skel.h, ${IDLS})

${IDL_SOURCES}: $(addsuffix .idl, ${IDLS})
	LD_LIBRARY_PATH=/home/pete/tests/OB1/lib /home/pete/tests/OB1/bin/idl $(addsuffix .idl, ${IDLS})
clean:
	rm -f *.o ${TARGETS} ${IDL_SOURCES} *~
