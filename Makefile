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


build/shaders.h: build 
	for file in src/shaders/*.fs; do \
		(cat "$file"; printf '\0') > "build/$(basename "$file")" \
		cp "$file" build/ \
	done
	cd build && \
	for file in *.fs; do \
		xxd -i "$$file" >> shaders.h; \
	done


build/ShapeUp: build build/shaders.h 
	$(CC) $(CCFLAGS)  src/main.c -o build/ShapeUp
	# src/pinchSwizzle.m


build:
	mkdir -p build

clean:
	rm -rf build

all: build/ShapeUp
