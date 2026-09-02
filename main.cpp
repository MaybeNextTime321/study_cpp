#include "main.h"

#include <cstdint>

int main(int argc, char** argv) // NOLINT(bugprone-exception-escape)
{
    if (argc == 1)
    {
        constexpr uint16_t serverPort = 9000;
        constexpr int startupFailureExitCode = 1;
        calculator::Application application;
        application.StartSignalHandling();

        try
        {
            calculator::NetworkServer server("0.0.0.0", serverPort);
            server.Start();

            while (!application.IsShutdownRequested())
            {
                std::this_thread::yield();
            }

            server.Stop();
        }
        catch (const boost::system::system_error& error)
        {
            utility::Logger::getInstance().log(
                std::string("Failed to start network server: ") + error.what(),
                utility::Logger::LogLevel::CRITICAL);
            application.StopSignalHandling();
            return startupFailureExitCode;
        }

        application.StopSignalHandling();
        return 0;
    }

    calculator::Application application;
    application.run(argc, argv);
}