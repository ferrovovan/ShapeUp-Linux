CC = gcc
CFLAGS = -std=c11 -g -Wall -Werror \
    -Wuninitialized -Wimplicit-fallthrough \
    -Winit-self -Wmissing-field-initializers \
    -Wno-declaration-after-statement \
    -Wno-error=deprecated-declarations \
    -Wno-error=unused-but-set-variable \
    -Wno-error=unused-function \
    -Wno-error=unused-label \
    -Wno-error=unused-variable \
    -Wno-missing-prototypes \
    -Wno-padded -Wno-pointer-sign \
    -Wno-sign-conversion \
    -Wshadow -Wstrict-prototypes \
-I build \
-I lib/raylib-4.5.0/include -L lib/raylib-4.5.0/lib \
-l -lX11


build/shaders.h: src/*.fs Makefile build
    for file in src/*.fs; do \
        (cat "$$file"; printf '\0') > "build/$$(basename "$$file")"; \
    done
    cd build && \
        for file in *.fs; do \
            xxd -i "$$file" >> shaders.h; \
        done


build/ShapeUp: src/* Makefile build/shaders.h build
	$(CC) $(CCFLAGS) src/pinchSwizzle.m src/main.c -o build/ShapeUp


build:
	mkdir -p build

clean:
	rm -rf build
