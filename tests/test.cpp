#include "../main.h"

#include <chrono>
#include <iostream>
#include <thread>

#include <gtest/gtest.h>

namespace
{
std::string WaitForResponse(calculator::NetworkClient& client,
                            const std::string& request,
                            std::chrono::milliseconds timeout)
{
    const auto deadline = std::chrono::steady_clock::now() + timeout;
    while (std::chrono::steady_clock::now() < deadline)
    {
        std::string response;
        if (client.SendRequest(request, response,
                               std::chrono::milliseconds(10)))
        {
            return response;
        }
        std::this_thread::yield();
    }
    return {};
}
} // namespace

TEST(HelpFunction, InputTest)
{
    char arg0[] = "calculation";
    char arg1[] = "-h";
    char* argv[] = {arg0, arg1};

    calculator::Application application;
    application.run(2, argv);
    EXPECT_EQ(math::MathStatus::Help, application.GetStatus());
}

TEST(NoFirstNumber, InputTest)
{
    char arg0[] = "calculation";
    char arg1[] = "{\"secondNumber\": 2, \"operation\":\"+\"}";
    char* argv[] = {arg0, arg1};

    calculator::Application application;
    application.run(2, argv);
    EXPECT_EQ(math::MathStatus::ParseError, application.GetStatus());
}

TEST(NoSecondNumber, InputTest)
{
    char arg0[] = "calculation";
    char arg1[] = "{\"firstNumber\": 2, \"operation\":\"+\"}";
    char* argv[] = {arg0, arg1};

    calculator::Application application;
    application.run(2, argv);
    EXPECT_EQ(math::MathStatus::ParseError, application.GetStatus());
}

TEST(NoOperation, InputTest)
{
    char arg0[] = "calculation";
    char arg1[] = "{\"firstNumber\": 2, \"secondNumber\": 3}";
    char* argv[] = {arg0, arg1};

    calculator::Application application;
    application.run(2, argv);
    EXPECT_EQ(math::MathStatus::ParseError, application.GetStatus());
}

TEST(PositiveSum, InputTest)
{
    char arg0[] = "calculation";
    char arg1[] =
        "{\"firstNumber\": 2, \"secondNumber\": 3, \"operation\": \"+\"}";
    char* argv[] = {arg0, arg1};

    calculator::Application application;
    application.run(2, argv);
    EXPECT_EQ(math::MathStatus::Ok, application.GetStatus());
    EXPECT_EQ(5, application.GetResult());
}

TEST(PositiveMinus, InputTest)
{
    char arg0[] = "calculation";
    char arg1[] =
        "{\"firstNumber\": 3, \"secondNumber\": 2, \"operation\": \"-\"}";
    char* argv[] = {arg0, arg1};

    calculator::Application application;
    application.run(2, argv);
    EXPECT_EQ(math::MathStatus::Ok, application.GetStatus());
    EXPECT_EQ(1, application.GetResult());
}

TEST(PositiveDivide, InputTest)
{
    char arg0[] = "calculation";
    char arg1[] =
        "{\"firstNumber\": 6, \"secondNumber\": 2, \"operation\": \"/\"}";
    char* argv[] = {arg0, arg1};

    calculator::Application application;
    application.run(2, argv);
    EXPECT_EQ(math::MathStatus::Ok, application.GetStatus());
    EXPECT_EQ(3, application.GetResult());
}

TEST(PositiveMultiply, InputTest)
{
    char arg0[] = "calculation";
    char arg1[] =
        "{\"firstNumber\": 3, \"secondNumber\": 2, \"operation\": \"*\"}";
    char* argv[] = {arg0, arg1};

    calculator::Application application;
    application.run(2, argv);
    EXPECT_EQ(math::MathStatus::Ok, application.GetStatus());
    EXPECT_EQ(6, application.GetResult());
}

TEST(PositiveFactorial, InputTest)
{
    char arg0[] = "calculation";
    char arg1[] =
        "{\"firstNumber\": 3, \"secondNumber\": 2, \"operation\": \"!\"}";
    char* argv[] = {arg0, arg1};

    calculator::Application application;
    application.run(2, argv);
    EXPECT_EQ(math::MathStatus::Ok, application.GetStatus());
    EXPECT_EQ(6, application.GetResult());
}

TEST(PositiveOverflow, InputTest)
{
    char arg0[] = "calculation";
    char arg1[] =
        "{\"firstNumber\": 30000, \"secondNumber\": 2, \"operation\": \"!\"}";
    char* argv[] = {arg0, arg1};

    calculator::Application application;
    application.run(2, argv);
    EXPECT_EQ(math::MathStatus::Overflow, application.GetStatus());
}

TEST(NegativeOverflow, SubtractionUnderflow)
{
    char arg0[] = "calculation";

    char arg1[] = "{\"firstNumber\": -2147483648, \"secondNumber\": 1, "
                  "\"operation\": \"-\"}";

    char* argv[] = {arg0, arg1};

    calculator::Application application;
    application.run(2, argv);

    EXPECT_EQ(math::MathStatus::Overflow, application.GetStatus());
}

TEST(DevideByZero, SubtractionUnderflow)
{
    char arg0[] = "calculation";

    char arg1[] = "{\"firstNumber\": 6, \"secondNumber\": 0, "
                  "\"operation\": \"/\"}";

    char* argv[] = {arg0, arg1};

    calculator::Application application;
    application.run(2, argv);

    EXPECT_EQ(math::MathStatus::DivideByZero, application.GetStatus());
}

TEST(FactorialFromNegative, SubtractionUnderflow)
{
    char arg0[] = "calculation";

    char arg1[] = "{\"firstNumber\": -6, \"secondNumber\": -1, "
                  "\"operation\": \"!\"}";

    char* argv[] = {arg0, arg1};

    calculator::Application application;
    application.run(2, argv);

    EXPECT_EQ(math::MathStatus::FactorialFromNegative, application.GetStatus());
}

TEST(SIGTERMHandling, SignalThreadSetsShutdownFlag)
{
    calculator::Application application;
    application.StartSignalHandling();

    EXPECT_FALSE(application.IsShutdownRequested());

    std::thread signalSender([]() { ::kill(getpid(), SIGTERM); });
    signalSender.join();

    const auto deadline =
        std::chrono::steady_clock::now() + std::chrono::milliseconds(500);
    while (!application.IsShutdownRequested() &&
           std::chrono::steady_clock::now() < deadline)
    {
        std::this_thread::yield();
    }

    EXPECT_TRUE(application.IsShutdownRequested());

    application.StopSignalHandling();
}

TEST(NetworkServer, HandlesAdditionRequest)
{
    calculator::NetworkServer server("127.0.0.1", 0);
    server.Start();

    const auto expectedPort = server.GetPort();
    ASSERT_GT(expectedPort, 0u);

    calculator::NetworkClient client("127.0.0.1", expectedPort);
    const std::string request =
        R"({"firstNumber": 5, "secondNumber": 3, "operation": "+"})";

    const std::string response =
        WaitForResponse(client, request, std::chrono::seconds(5));

    ASSERT_FALSE(response.empty());
    EXPECT_NE(response.find("\"result\":8"), std::string::npos);

    server.Stop();
}