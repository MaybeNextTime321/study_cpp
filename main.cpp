#include "main.h"

int main(int argc, char** argv) // NOLINT(bugprone-exception-escape)
{
    calculator::Application application;
    application.run(argc, argv);
}