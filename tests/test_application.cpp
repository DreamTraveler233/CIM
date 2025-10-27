#include "application.hpp"

int main(int argc, char **argv)
{
    CIM::Application app;
    if (app.init(argc, argv))
    {
        return app.run();
    }
    return 0;
}