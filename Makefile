RAY = vendor/raylib-src/src
RAYLIB_URL = https://github.com/raysan5/raylib/archive/refs/tags/5.5.tar.gz

demo: demo.c $(RAY)/libraylib.a
	cc -O2 -std=c99 -Wall -I$(RAY) demo.c $(RAY)/libraylib.a -lm \
	  -framework IOKit -framework Cocoa -framework OpenGL -o demo

# raylib 5.5 is downloaded on first build
$(RAY)/raylib.h:
	mkdir -p vendor/raylib-src
	curl -L $(RAYLIB_URL) | tar xz --strip-components=1 -C vendor/raylib-src

$(RAY)/libraylib.a: $(RAY)/raylib.h
	$(MAKE) -C $(RAY) PLATFORM=PLATFORM_DESKTOP

# make run LANG_ARG=it   (en, it, fr, es)
LANG_ARG ?= en
run: demo
	./demo --lang $(LANG_ARG)

clean:
	rm -f demo
