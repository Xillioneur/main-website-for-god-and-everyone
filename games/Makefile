# --- THE DIVINE BUILD SYSTEM v2.6 (iOS Compatible) ---

RAYLIB_PATH = $(HOME)/raylib
RAYLIB_SRC_PATH = $(RAYLIB_PATH)/src
LIBRAYLIB_PATH = $(RAYLIB_SRC_PATH)/libraylib.web.a

EMCC = emcc

# Divine Compilation Flags
# Disabled -pthread for mobile/iOS compatibility (Avoids SharedArrayBuffer security requirements)
CFLAGS = -O2 -std=c++23 -I$(RAYLIB_SRC_PATH) -L$(RAYLIB_SRC_PATH) -DGRAPHICS_API_OPENGL_ES3

# Manifestation Flags
# Removed -s USE_PTHREADS=1: iOS/Chrome iOS requires strict COOP/COEP headers for pthreads/SharedArrayBuffer.
# Forcing single-threaded mode for maximum portability.
EMCC_FLAGS = -s USE_GLFW=3 -s ASYNCIFY -s FORCE_FILESYSTEM=1 -s USE_SDL=2 \
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
