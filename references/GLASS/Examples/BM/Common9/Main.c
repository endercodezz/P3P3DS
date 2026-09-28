#include <arm.h>

int main(void) {
    while (true)
        __wfi();

    return 0;
}