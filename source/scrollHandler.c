#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <stdio.h>

float magnification = 0;

void processScrollEvent(XEvent *event) {
    if (event->type == ButtonPress) {
        XButtonPressedEvent *buttonEvent = (XButtonPressedEvent *)event;
        if (buttonEvent->button == Button4) {
            // Scroll up
            magnification += 0.1;
            printf("Magnification: %f\n", magnification);
        } else if (buttonEvent->button == Button5) {
            // Scroll down
            magnification -= 0.1;
            printf("Magnification: %f\n", magnification);
        }
    }
}

int main(void) {
    Display *display;
    Window rootWindow;

    display = XOpenDisplay(NULL);
    if (display == NULL) {
        fprintf(stderr, "Error: Could not open display.\n");
        return 1;
    }

    rootWindow = DefaultRootWindow(display);
    XSelectInput(display, rootWindow, ButtonPressMask);

    XEvent event;
    while (1) {
        XNextEvent(display, &event);
        processScrollEvent(&event);
    }

    XCloseDisplay(display);
    return 0;
}

