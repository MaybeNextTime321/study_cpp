#pragma once
#include "spdlog/spdlog.h"

#include <customMath.h>
#include <getopt.h>
#include <unistd.h>

#include <nlohmann/json.hpp>

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>

#include "cmath"

namespace utility
{
class Logger
{
  public:
    static Logger& getInstance()
    {
        static Logger instance;
        return instance;
    }

    enum LogLevel : uint8_t
    {
        INFO,
        DEBUG,
        WARN,
        CRITICAL,
        ERROR
    };

    void log( // NOLINT(readability-convert-member-functions-to-static)
        const std::string& msg, LogLevel logLever)
    {
        switch (logLever)
        {
            case LogLevel::INFO:
                spdlog::info(msg);
                break;

            case LogLevel::DEBUG:
                spdlog::debug(msg);
                break;

            case LogLevel::WARN:
                spdlog::warn(msg);
                break;

            case LogLevel::CRITICAL:
                spdlog::critical(msg);
                break;

            case LogLevel::ERROR:
                spdlog::error(msg);
                break;

            default:
                break;
        }
    }
};
} // namespace utility

namespace calculator
{

class Application
{
  public:
    Application() : task_(Task()){};

    double GetResult() const
    {
        return task_.result;
    }

    char GetOperation() const
    {
        return task_.operation;
    }

    double GetFirstNumber() const
    {
        return task_.firstNumber;
    }

    double GetSecondNumber() const
    {
        return task_.secondNumber;
    }

    math::MathStatus GetStatus() const
    {
        return task_.operationStatus;
    }

    void run(int argc, char** argv)
    {
        makeTask(argc, argv);
        makeCalculate();
        printResult();
    }

  private:
    template <typename T>
    void parseVariableToValue(nlohmann::json& json, T& variableToWrite,
                              const std::string& key)
    {
        try
        {
            variableToWrite = json[key].get<T>();
        }
        catch (...)
        {
            utility::Logger::getInstance().log(
                std::string("Error while loading " + key +
                            " in JSON. Please check you input"),
                utility::Logger::LogLevel::CRITICAL);
            task_.operationStatus = math::MathStatus::ParseError;
        }
    }

    void makeCalculate()
    {
        if (task_.operationStatus != math::MathStatus::Ok)
        {
            return;
        }

        switch (task_.operation)
        {
            case '+':
                task_.result = math::add(task_.firstNumber, task_.secondNumber,
                                         task_.operationStatus);
                break;
            case '-':
                task_.result =
                    math::substract(task_.firstNumber, task_.secondNumber,
                                    task_.operationStatus);
                break;
            case '*':
                task_.result =
                    math::multiply(task_.firstNumber, task_.secondNumber,
                                   task_.operationStatus);
                break;
            case '/':
                task_.result =
                    math::divide(task_.firstNumber, task_.secondNumber,
                                 task_.operationStatus);
                break;
            case '^':
                task_.result =
                    math::power(task_.firstNumber, task_.secondNumber,
                                task_.operationStatus);
                break;
            case '!':
                task_.result =
                    math::factorial(task_.firstNumber, task_.operationStatus);
                break;
            default:
                break;
        }
    }

    void printResult() const
    {
        switch (task_.operationStatus)
        {
            case math::MathStatus::Ok:
                utility::Logger::getInstance().log(
                    "Operation status: OK", utility::Logger::LogLevel::INFO);
                utility::Logger::getInstance().log(
                    std::string("Result is: ") + std::to_string(task_.result),
                    utility::Logger::LogLevel::INFO);
                break;
            case math::MathStatus::DivideByZero:
                utility::Logger::getInstance().log(
                    "DivideByZero", utility::Logger::LogLevel::ERROR);
                break;
            case math::MathStatus::Overflow:
                utility::Logger::getInstance().log(
                    "Overflow", utility::Logger::LogLevel::ERROR);
                break;
            case math::MathStatus::FactorialFromNegative:
                utility::Logger::getInstance().log(
                    "FactorialFromNegative", utility::Logger::LogLevel::ERROR);
                break;
            case math::MathStatus::ParseError:
                utility::Logger::getInstance().log(
                    "ParseError", utility::Logger::LogLevel::ERROR);
                break;
            case math::MathStatus::Help:
                printHelp();
                break;
        }
    }

    bool parseOperation(
        const char*
            optarg) // NOLINT(readability-convert-member-functions-to-static,-warnings-as-errors)
    {
        const char operation = *optarg;
        switch (operation)
        {
            case '+':
            case '-':
            case '*':
            case '/':
            case '^':
            case '!':
                utility::Logger::getInstance().log(
                    std::string("Parsed op::") + operation,
                    utility::Logger::LogLevel::INFO);
                task_.operation = operation;
                return true;
            default:
                throw("Operator is empty or couldn't be parsed");
                return false;
        }
    }

    void makeTask(const int& argc, char* argv[])
    {
        int arg = 0;

        static struct option longOptions[] = {
            {"help", no_argument, nullptr, 'h'}, {nullptr, 0, nullptr, 0}};

        int optionIndex = 0;
        arg = getopt_long( // NOLINT(concurrency-mt-unsafe, cppcoreguidelines-pro-bounds-array-to-pointer-decay)
            argc, argv, "h",
            longOptions, // NOLINT(concurrency-mt-unsafe, cppcoreguidelines-pro-bounds-array-to-pointer-decay)
            &optionIndex);

        if (arg == 'h')
        {
            task_.operationStatus = math::MathStatus::Help;
            return;
        }

        nlohmann::json jsonInput;

        try
        {
            jsonInput = nlohmann::json::parse(
                argv[1]); // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic)
        }
        catch (...)
        {
            utility::Logger::getInstance().log(
                std::string("Error while loading JSON. Please check you input"),
                utility::Logger::LogLevel::CRITICAL);
            task_.operationStatus = math::MathStatus::ParseError;
            return;
        }

        parseVariableToValue(jsonInput, task_.firstNumber, "firstNumber");
        parseVariableToValue(jsonInput, task_.secondNumber, "secondNumber");
        std::string operationString{};
        parseVariableToValue(jsonInput, operationString, "operation");
        task_.operation = operationString[0];
    }

    void printHelp() // NOLINT(readability-convert-member-functions-to-static)
        const
    {
        utility::Logger::getInstance().log("Usage:",
                                           utility::Logger::LogLevel::INFO);
        utility::Logger::getInstance().log("  calculation '<JSON string>'",
                                           utility::Logger::LogLevel::INFO);
        utility::Logger::getInstance().log("JSON fields:",
                                           utility::Logger::LogLevel::INFO);
        utility::Logger::getInstance().log(
            "  firstNumber  — first operand (integer)",
            utility::Logger::LogLevel::INFO);
        utility::Logger::getInstance().log(
            "  secondNumber — second operand (integer)",
            utility::Logger::LogLevel::INFO);
        utility::Logger::getInstance().log(
            "  operation    — operator (+, -, *, /, ^, !)",
            utility::Logger::LogLevel::INFO);
        utility::Logger::getInstance().log("Example:",
                                           utility::Logger::LogLevel::INFO);
        utility::Logger::getInstance().log(
            "  calculation '{\"firstNumber\": 5, \"secondNumber\": 3, "
            "\"operation\": \"+\"}'",
            utility::Logger::LogLevel::INFO);
        utility::Logger::getInstance().log(
            "Note: wrap JSON in single quotes to prevent shell expansion",
            utility::Logger::LogLevel::INFO);
    }
    struct Task
    {
        int firstNumber{0};
        int secondNumber{0};
        char operation{0};
        double result{0.0};
        math::MathStatus operationStatus{math::MathStatus::Ok};
    };

  private: // NOLINT(readability-redundant-access-specifiers)
    Task task_;
};

} // namespace calculator