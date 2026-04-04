#include"Core.h"

int main(){
    Core::init();
    Core::mainLoop();
    Core::quit();
    return 0;
}