#include "MyApp.hpp"
#include <cstdlib>

MyApp::MyApp(int width, int height, const char* title) : 
            App(width, height, title)
{

}

void MyApp::initDraw(){

}

void MyApp::draw(){

}

void MyApp::processInput(){
    if(keyPressed(GLFW_KEY_ESCAPE)){
        terminate();
    }
}

void MyApp::processMouse(){

}

int main(){
    MyApp myapp(1024, 768, "VK");
    myapp.start();
    return EXIT_SUCCESS;
}
