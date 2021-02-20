#config
DEBUG=1

#setup
SOURCES=
LIBS=
INCLUDE=

#header includes
INCLUDE += src

#source includes
SOURCES += src/main.cpp
SOURCES += src/GameServer.cpp
SOURCES += src/Vector.cpp
SOURCES += src/Pod.cpp
SOURCES += src/Player.cpp


#lib includes

#more setup
EXECUTABLE=out/podracing.exe

ifeq ($(DEBUG), 1)
	FLAG_BUILD_MODE=-O0 -ggdb3
else
	FLAG_BUILD_MODE=-O3
endif

LDFLAGS=-Wall $(FLAG_BUILD_MODE)
CC=g++
CFLAGS=-c -MMD -Wall $(FLAG_BUILD_MODE)
OBJECTS=$(SOURCES:%.cpp=out/%.o)
DEPENDENCIES=$(OBJECTS_FINAL:.o=.d)

INCLUDE_FORMATTED=$(addprefix -I, $(INCLUDE))

#targets
.PHONY: all
all: $(EXECUTABLE)

$(EXECUTABLE): $(OBJECTS)
	@$(CC) $(LDFLAGS) $(OBJECTS) $(LIBS) -o $@
	@echo $@

$(OBJECTS): out/%.o : %.cpp
	@mkdir -p out/$(dir $<)
	@$(CC) $(CFLAGS) $(INCLUDE_FORMATTED) $< -o $@
	@echo $<


.PHONY: clean
clean:
	@rm -rf out/*

-include $(DEPENDENCIES)