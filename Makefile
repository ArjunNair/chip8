#CPPFLAGS lists all the include paths
CPPFLAGS = -I. \
		 	-I./imgui \
		 	-I"D:\Programs\SDL2-2.0.12\x86_64-w64-mingw32\include\SDL2"

#LDFLAGS lists all the library paths
LDFLAGS = -L"D:\Programs\SDL2-2.0.12\x86_64-w64-mingw32\lib"\
		-L"D:\Programs\msys2\mingw64\lib"

#The name of the executable
EXEC = Chimp 

#The source files in the project
SRC_FILES = Chip8.cpp Chip8Sound.cpp CTexture.cpp main.cpp ./imgui_impl_sdl.cpp ./imgui_impl_opengl2.cpp ./imgui/imgui*.cpp

#The compiler to use
CXX = g++
CC = $(CXX)

#Flags for the compiler
DEBUG_LEVEL     = -g

#Remove -Wl,--subsystem,windows if console window *is* required.
#Otherwise this flag removes the console window.
#EXTRA_CCFLAGS   = -Wl,--subsystem,windows
CXX_VERSION     = -std=c++17
CXXFLAGS        = $(DEBUG_LEVEL) $(CXX_VERSION) $(EXTRA_CCFLAGS)
CCFLAGS         = $(CXXFLAGS) 

#The output directory for the executable
OUTPUT_DIR		= ./Debug/

#LDLIBS lists alls the libraries that need to be linked in
ifeq ($(OS),Windows_NT)
	LDLIBS = -lmingw32 -lSDL2main -lSDL2 -lopengl32
else
	UNAME_S := $(shell uname -s)
	ifeq ($(UNAME_S),Darwin)
		LDLIBS = -framework OpenGL -framework Cocoa
	else ifeq ($(UNAME_S),Linux)
		LDLIBS = -lGLU -lGL -lX11 -lSDL2main -lSDL2
	endif
endif

#The target all is same as the name of executable
all: $(EXEC)
	
#The actual target
$(EXEC):
	$(CC) $(CPPFLAGS) $(CCFLAGS) $(SRC_FILES) $(LDFLAGS) $(LDLIBS) -o $(OUTPUT_DIR)$(EXEC)

clean:
	$(RM) $(OUTPUT_DIR)$(EXEC)

