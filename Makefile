ifeq ($(BUILD), wince)
	TOOLCHAIN_DIR=/opt/mingw32ce/bin
	CXX=$(TOOLCHAIN_DIR)/arm-mingw32ce-g++
	CC=$(TOOLCHAIN_DIR)/arm-mingw32ce-gcc
	LD=$(TOOLCHAIN_DIR)/arm-mingw32ce-gcc
	TARGET=wince
else ifeq ($(BUILD), win32)
	CXX=i686-w64-mingw32-g++
	CC=i686-w64-mingw32-gcc
	LD=i686-w64-mingw32-gc
	TARGET=win32
else
	CXX=g++
	CC=gcc
	LD=gcc
	TARGET=gles2 glx
endif

SDL_ROOT_DIR=/storage/sources/GIT/SDL/
SDL_INCLUDE=$(SDL_ROOT_DIR)/include
SDL_LIB=$(SDL_ROOT_DIR)/lib

# Install
LIBRARY_GLES2 = bin/libnuklear_gles2.so
LIBRARY_GLX = bin/libnuklear_glx.so
TEST_GLES2 = bin/test_gles2
TEST_GLX = bin/test_glx
TEST_WINCE = bin/test_wince.exe
TEST_WIN32 = bin/test_win32.exe
# Flags
INCLUDE_DIR=-Iinclude -Iothers
CFLAGS_BASE=-pedantic -ggdb $(INCLUDE_DIR)
CFLAGS_GLES2 +=$(CFLAGS_BASE) -I$(SDL_INCLUDE) -L$(SDL_LIB) -DNUKLEAR_GLES2 -std=c++11
CFLAGS_GLX +=$(CFLAGS_BASE) -DNUKLEAR_GLX -std=c++11
CFLAGS_WINCE +=$(CFLAGS_BASE) -gstabs -DNUKLEAR_WINCE -DWINCE -DWINBUILD -D_WIN32_WCE=0x400  -D_WIN32_IE=0x400 -DWINVER=0x400 -D__USE_W32_SOCKETS  -DUNDER_CE -D_UNICODE #-D__CEGCC__
CFLAGS_WIN32 +=$(CFLAGS_BASE) -gstabs -DNUKLEAR_WINCE -DWIN32 -DWINBUILD -D__USE_W32_SOCKETS -D_UNICODE
BASE_SRC_LIB = src/window.cc src/nuklear_lib.cpp src/thread.cc src/timer.cc 
SRC_LIB_GLES2 = src/window_gles2.cc $(BASE_SRC_LIB)
SRC_LIB_GLX = src/window_glx.cc $(BASE_SRC_LIB)
SRC_LIB_WIN = src/window.cc src/nuklear_lib.cpp src/timer.cc src/window_gdi.cc test/main.cc
SRC_MAIN = test/main.cc


all: $(TARGET)

clean:
	rm $(LIBRARY_GLES2) $(LIBRARY_GL) $(TEST_GLES2) $(TEST_GLX) 

glx: $(TEST_GLX)
gles2: $(TEST_GLES2)

$(LIBRARY_GLES2): $(SRC_LIB_GLES2) include/nuklear.h
	@mkdir -p bin
	$(CXX) -shared $(SRC_LIB_GLES2) $(CFLAGS_GLES2) -o $(LIBRARY_GLES2) -I$(INCLUDE_DIR) 
	
$(LIBRARY_GLX): $(SRC_LIB_GLX) include/nuklear.h
	@mkdir -p bin
	$(CXX) -shared $(SRC_LIB_GLX) $(CFLAGS_GLX) -o $(LIBRARY_GLX) -I$(INCLUDE_DIR) -fPIC
	
$(TEST_GLES2): $(SRC_MAIN) $(LIBRARY_GLES2)
	$(CXX) $(SRC_MAIN) $(CFLAGS) -o $(TEST_GLES2) $(CFLAGS_GLES2) -Lbin -lnuklear_gles2 -lm -lSDL2 -lGLESv2 -lGLdispatch -lpthread  -fPIC

$(TEST_GLX): $(SRC_MAIN) $(LIBRARY_GLX)
	$(CXX) $(SRC_MAIN) $(CFLAGS_GLX) -o $(TEST_GLX) -Lbin -lnuklear_glx -lm -x11 -lGLX -lGL -lGLdispatch -lpthread

$(TEST_WINCE): $(SRC_LIB_WINCE)
	$(CXX) $(CFLAGS_WINCE) $(SRC_LIB_WIN) -o $(TEST_WINCE) -Lbin -static-libgcc -Wl,-Bstatic -lstdc++

$(TEST_WIN32): $(SRC_LIB_WINCE)
	$(CXX) $(CFLAGS_WIN32) $(SRC_LIB_WIN) -o $(TEST_WIN32) -Lbin -lgdi32 -static-libgcc -static-libstdc++ -Wl,-Bstatic -lstdc++

wince: $(TEST_WINCE)

win32: $(TEST_WIN32)
