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

build/ShapeUp: build build/shaders.h 
	$(CC) $(CCFLAGS)  source/main.c -o build/ShapeUp
	# src/pinchSwizzle.m


build/shaders.h: build source/shaders/*.fs
	(cat source/shaders/shader_base.fs; printf '\0') > build/shader_base.fs
	(cat source/shaders/shader_prefix.fs; printf '\0') > build/shader_prefix.fs
	(cat source/shaders/slicer_body.fs; printf '\0') > build/slicer_body.fs
	(cat source/shaders/selection.fs; printf '\0') > build/selection.fs
	cd build && xxd -i shader_base.fs shaders.h
	cd build && xxd -i shader_prefix.fs >> shaders.h
	cd build && xxd -i slicer_body.fs >> shaders.h
	cd build && xxd -i selection.fs >> shaders.h


build:
	mkdir -p build

clean:
	rm -rf build
