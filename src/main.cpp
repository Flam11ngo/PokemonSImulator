#include<Core.h>

int main(int argc, char* argv[]) {
    Core::init();
    Core::mainLoop();
    Core::quit();
    return 0;
}