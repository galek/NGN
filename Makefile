# Links:
# http://mrbook.org/blog/tutorials/make/
# http://www.ijon.de/comp/tutorials/makefile.html
# http://www.cs.colby.edu/maxwell/courses/tutorials/maketutor/
# http://stackoverflow.com/questions/1079832/how-can-i-configure-my-makefile-for-debug-and-release-builds
# Reminder:
# $< first dependency
# $@ name of the target
# $^ list of dependencies, unique ($+ is the non-unique list of dependencies)

CC = g++
CFLAGS = -Wall -Isrc/ -Isrc/ngn -std=gnu++11
EXECUTABLE = build/ngnTest
LDFLAGS =

SRC = src/main.cpp src/ngn/log.cpp src/ngn/window.cpp src/ngn/object.cpp src/ngn/renderer.cpp
OBJ = $(SRC:%.cpp=%.o)

# dependencies
## SDL
CFLAGS += -Idependencies/debug/SDL/include/SDL2
LDFLAGS += -Ldependencies/debug/SDL/lib -lmingw32 -lSDL2main -lSDL2
## OpenGL
LDFLAGS += -lopengl32
## GLAD
CFLAGS += -Idependencies/glad/include
LDFLAGS += -Ldependencies/glad/src -lglad
# GLM
CFLAGS += -Idependencies/glm

LDFLAGS += -lstdc++

all: test

debug: CFLAGS += -DDEBUG -g -O0
debug: $(EXECUTABLE)

release: CFLAGS += -DNDEBUG -O3
release: $(EXECUTABLE)

$(EXECUTABLE): $(OBJ)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

%.o: %.cpp
	$(CC) $(CFLAGS) -c $< -o $@

run:
	$(EXECUTABLE)

test: debug run

remake: clean all

clean:
	rm -f $(EXECUTABLE) $(OBJ)