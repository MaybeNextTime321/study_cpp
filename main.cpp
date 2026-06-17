#include "spdlog/spdlog.h"

#include <customMath.h>
#include <getopt.h>
#include <unistd.h>

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

    void run(int argc, char** argv)
    {
        makeTask(argc, argv);
        makeCalculate();
        printResult();
    }

  private:
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

    bool
        charToInt( // NOLINT(readability-convert-member-functions-to-static,-warnings-as-errors)
            char* optarg, int& variableToWrite)
    {

        char* end = nullptr;
        const int16_t numberSystem = 10;
        int64_t parsed = 0;
        parsed = strtol(optarg, &end, numberSystem);
        if (end == optarg || *end != '\0')
        {
            throw("Par is empty or couldn't be parsed");
        }

        variableToWrite = static_cast<int>(parsed);
        utility::Logger::getInstance().log(std::string("Parsed par:") +
                                               std::to_string(variableToWrite),
                                           utility::Logger::LogLevel::INFO);
        return true;
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
            {"help", no_argument, nullptr, 'h'},
            {"firstNumber", required_argument, nullptr, 'f'},
            {"secondNumber", required_argument, nullptr, 's'},
            {"operation", required_argument, nullptr, 'o'},
            {nullptr, 0, nullptr, 0}};

        while (true)
        {
            int optionIndex = 0;
            arg = getopt_long( // NOLINT(concurrency-mt-unsafe, cppcoreguidelines-pro-bounds-array-to-pointer-decay)
                argc, argv, "f:s:o:",
                longOptions, // NOLINT(concurrency-mt-unsafe, cppcoreguidelines-pro-bounds-array-to-pointer-decay)
                &optionIndex);

            if (arg == -1)
            {
                break;
            }

            switch (arg)
            {
                case 'h':
                    task_.operationStatus = math::MathStatus::Help;
                    return;
                    break;
                case 'f':
                    try
                    {
                        charToInt(optarg, task_.firstNumber);
                    }
                    catch (const char* error)
                    {
                        utility::Logger::getInstance().log(
                            std::string("Error while parsing First number: ") +
                                error,
                            utility::Logger::LogLevel::ERROR);
                        task_.operationStatus = math::MathStatus::ParseError;
                        return;
                    }
                    catch (...) // NOLINT(bugprone-empty-catch)
                    {
                        utility::Logger::getInstance().log(
                            "Unknown error while parsing First number",
                            utility::Logger::LogLevel::ERROR);
                    }
                    break;
                case 's':
                    try
                    {
                        charToInt(optarg, task_.secondNumber);
                    }
                    catch (const char* error)
                    {
                        utility::Logger::getInstance().log(
                            std::string("Error while parsing Second number: ") +
                                error,
                            utility::Logger::LogLevel::ERROR);
                        task_.operationStatus = math::MathStatus::ParseError;
                        return;
                    }
                    catch (...) // NOLINT(bugprone-empty-catch)
                    {
                        utility::Logger::getInstance().log(
                            "Unknown error while parsing Second number",
                            utility::Logger::LogLevel::ERROR);
                    }

                    break;
                case 'o':
                    try
                    {
                        parseOperation(optarg);
                    }
                    catch (const char* error)
                    {
                        utility::Logger::getInstance().log(
                            std::string("Error while parsing Operator: ") +
                                error,
                            utility::Logger::LogLevel::ERROR);
                        task_.operationStatus = math::MathStatus::ParseError;
                        return;
                    }
                    catch (...) // NOLINT(bugprone-empty-catch)
                    {
                        utility::Logger::getInstance().log(
                            "Unknown error while parsing operator",
                            utility::Logger::LogLevel::ERROR);
                    }
                    break;
                case '?':
                    break;
                default:
                    abort();
            }
        }
    }

    void printHelp() // NOLINT(readability-convert-member-functions-to-static)
        const
    {
        utility::Logger::getInstance().log("Options:",
                                           utility::Logger::LogLevel::INFO);
        utility::Logger::getInstance().log("--firstNumber,  -f  First number",
                                           utility::Logger::LogLevel::INFO);
        utility::Logger::getInstance().log("--secondNumber, -s  Second number",
                                           utility::Logger::LogLevel::INFO);
        utility::Logger::getInstance().log(
            "--operation,    -o  Operation (+, -, *, /, ^, !, %%)",
            utility::Logger::LogLevel::INFO);
        utility::Logger::getInstance().log("--help,         -h  Show this help",
                                           utility::Logger::LogLevel::INFO);
        utility::Logger::getInstance().log("Example",
                                           utility::Logger::LogLevel::INFO);
        utility::Logger::getInstance().log(
            "calculation --firstNumber 5 --secondNumber 3 --operation +",
            utility::Logger::LogLevel::INFO);
        utility::Logger::getInstance().log(
            "Note: use quotes for * operator: --operation '*'",
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

int main(int argc, char** argv)
{
    calculator::Application application;
    application.run(argc, argv);
}