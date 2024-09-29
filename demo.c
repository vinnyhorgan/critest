#include <stdio.h>

#define CRI_IMPL
#include "cri.h"

static void on_error(int error, const char *description) {
    printf("error %d: %s\n", error, description);
}

int main() {
    cri_Window *window;

    cri_set_error_callback(on_error);

    if (!cri_init())
        return 1;

    window = cri_create_window(640, 480, "Hello cri");
    if (!window)
        return 1;

    while (!cri_window_should_close(window)) {
        cri_poll_events();
    }

    cri_destroy_window(window);
    cri_terminate();

    return 0;
}
