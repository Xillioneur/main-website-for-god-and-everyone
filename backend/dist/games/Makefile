# --- THE DIVINE BUILD SYSTEM v2.5 ---
# Hybrid Threading: Main loop on UI thread, computation on Workers

RAYLIB_PATH = $(HOME)/raylib
RAYLIB_SRC_PATH = $(RAYLIB_PATH)/src
LIBRAYLIB_PATH = $(RAYLIB_SRC_PATH)/libraylib.web.a

EMCC = emcc

# Divine Compilation Flags
# -pthread: Enable the threads of creation
# -DGRAPHICS_API_OPENGL_ES3: Match the modern graphics pipeline
CFLAGS = -O2 -std=c++23 -pthread -I$(RAYLIB_SRC_PATH) -L$(RAYLIB_SRC_PATH) -DGRAPHICS_API_OPENGL_ES3

# Manifestation Flags
# Removed -s PROXY_TO_PTHREAD=1: We keep main() on the main thread to access DOM/Window/GLFW.
# Kept -s USE_PTHREADS=1: We still allow std::thread to spawn workers for the ThreadPool.
EMCC_FLAGS = -s USE_GLFW=3 -s ASYNCIFY -s FORCE_FILESYSTEM=1 -s USE_SDL=2 \
             -s USE_PTHREADS=1 -s PTHREAD_POOL_SIZE=4 \
             -s MAX_WEBGL_VERSION=2 -s MIN_WEBGL_VERSION=2 \
             --shell-file ../game_shell.html

GAMES = $(shell find . -mindepth 1 -maxdepth 1 -type d)

.PHONY: all clean $(GAMES)

all: $(GAMES)

$(GAMES):
	@echo "--------------------------------------------------"
	@echo "MANIFESTING: $(notdir $@)"
	@echo "--------------------------------------------------"
	@cd $@ && \
	SOURCES=$$(ls *.cpp 2>/dev/null); \
	PRELOAD=$$(if [ -d resources ]; then echo "--preload-file resources"; fi); \
	$(EMCC) $$SOURCES -o $(notdir $@).html $(CFLAGS) $(LIBRAYLIB_PATH) $(EMCC_FLAGS) $$PRELOAD

clean:
	@for dir in $(GAMES); do \
		echo "Cleaning $$dir"; \
		rm -f $$dir/*.html $$dir/*.js $$dir/*.wasm $$dir/*.data; \
	done