SDL_ROOT_DIR=/storage/sources/GIT/SDL/
SDL_INCLUDE=$(SDL_ROOT_DIR)/include
SDL_LIB=$(SDL_ROOT_DIR)/lib

# Install
LIBRARY_GLES2 = bin/libnuklear_gles2.so
LIBRARY_GLX = bin/libnuklear_glx.so
TEST_GLES2 = bin/test_gles2
TEST_GLX = bin/test_glx
# Flags
INCLUDE_DIR=-Iinclude -Iothers
CFLAGS_BASE=-pedantic -ggdb $(INCLUDE_DIR) -std=c++11
CFLAGS_GLES2 +=$(CFLAGS_BASE) -I$(SDL_INCLUDE) -L$(SDL_LIB) -DNUKLEAR_GLES2
CFLAGS_GLX +=$(CFLAGS_BASE) -DNUKLEAR_GLX
BASE_SRC_LIB = src/window.cc src/nuklear_lib.cpp src/process.cc src/thread.cc src/timer.cc
SRC_LIB_GLES2 = src/window_gles2.cc $(BASE_SRC_LIB)
SRC_LIB_GLX = src/window_glx.cc $(BASE_SRC_LIB)
SRC_MAIN = test/main.cc
BITSCOPE_LIB=-lBitLib

all: $(TEST_GLES2) $(TEST_GLX) 

clean:
	rm $(LIBRARY_GLES2) $(LIBRARY_GL) $(TEST_GLES2) $(TEST_GLX) 

glx: $(TEST_GLX)
gles2: $(TEST_GLES2)

$(LIBRARY_GLES2): $(SRC_LIB_GLES2) include/nuklear.h
	@mkdir -p bin
	$(CXX) -shared $(SRC_LIB_GLES2) $(CFLAGS_GLES2) -o $(LIBRARY_GLES2) -I$(INCLUDE_DIR) -fPIC 
	
$(LIBRARY_GLX): $(SRC_LIB_GLX) include/nuklear.h
	@mkdir -p bin
	$(CXX) -shared $(SRC_LIB_GLX) $(CFLAGS_GLX) -o $(LIBRARY_GLX) -I$(INCLUDE_DIR) -fPIC 
	
$(TEST_GLES2): $(SRC_MAIN) $(LIBRARY_GLES2)
	$(CXX) $(SRC_MAIN) $(CFLAGS) -o $(TEST_GLES2) $(CFLAGS_GLES2) $(BITSCOPE_LIB) -Lbin -lnuklear_gles2 -lm -lSDL2 -lGLESv2 -lGLdispatch -lpthread

$(TEST_GLX): $(SRC_MAIN) $(LIBRARY_GLX)
	$(CXX) $(SRC_MAIN) $(CFLAGS_GLX) -o $(TEST_GLX) $(BITSCOPE_LIB) -Lbin -lnuklear_glx -lm -x11 -lGLX -lGL -lGLdispatch -lpthread