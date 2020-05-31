#include <iostream>

#include "window.h"

int main(void)
{
    try
    {
        Window w{800, 600, "Sample"};
        w.create();
        w.initScene();
        w.run();
    }
    catch(const std::exception & e)
    {
        std::cout << "ERROR: " << e.what() << std::endl;
    }

    return 0;
}
