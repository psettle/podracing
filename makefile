#config
DEBUG=0

#setup
SOURCES=
LIBS=
INCLUDE=

#header includes
INCLUDE += src
INCLUDE += src/engine
INCLUDE += src/neurons
INCLUDE += src/genetics
INCLUDE += src/controller

#source includes
SOURCES += src/main.cpp
SOURCES += src/engine/GameServer.cpp
SOURCES += src/engine/Pod.cpp
SOURCES += src/engine/Player.cpp
SOURCES += src/neurons/NeuralNetwork.cpp
SOURCES += src/genetics/NeuralNetworkFactory.cpp
SOURCES += src/controller/DualAdvancedRunner.cpp
SOURCES += src/controller/TrainedNetworks.cpp


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