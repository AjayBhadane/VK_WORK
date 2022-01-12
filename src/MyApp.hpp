#include "Application.hpp"

class MyApp : public App {
    public:
        MyApp(int, int, const char* title);
        void initDraw();
        void draw();
    private:
        void processInput();
        void processMouse();
};