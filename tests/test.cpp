#include "../main.h"

#include <iostream>

#include <gtest/gtest.h>

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