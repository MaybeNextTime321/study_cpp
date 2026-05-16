#include <customMath.h>
#include <getopt.h>
#include <unistd.h>

#include <cstdio>
#include <cstdlib>
#include <iostream>

#include "cmath"

namespace utility
{
struct Task
{
    int firstNumber{0};
    int secondNumber{0};
    char operation{0};
    double result{0.0};
    math::MathStatus operationStatus{math::MathStatus::Ok};
};
} // namespace utility

bool charToInt(char* optarg, int& variableToWrite)
{

    char* end = nullptr;
    const int16_t numberSystem = 10;
    int64_t parsed = 0;
    parsed = strtol(optarg, &end, numberSystem);
    if (end == optarg || *end != '\0')
    {
        std::cout << "Error while parsing par: " << optarg << '\n';
        return false;
    }

    variableToWrite = static_cast<int>(parsed);
    std::cout << "Parsed par: " << variableToWrite << '\n';
    return true;
}

bool parseOperation(const char* optarg, char& operationVariable)
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
            std::cout << "Parsed op: " << operation << '\n';
            operationVariable = operation;
            return true;
        default:
            std::cout << "Error while parsing op: " << operation << '\n';
            return false;
    }
}

void printHelp()
{
    std::cout << "Usage my calculation " << " [OPTIONS]\n"
              << "Options:\n"
              << "  --firstNumber,  -f  First number\n"
              << "  --secondNumber, -s  Second number\n"
              << "  --operation,    -o  Operation (+, -, *, /, ^, !, %)\n"
              << "  --help,         -h  Show this help\n"
              << "Example:\n"
              << "  " << "calculation"
              << " --firstNumber 5 --secondNumber 3 --operation +\n"
              << "  Note: use quotes for * operator: --operation '*'\n";
}

void printResult(utility::Task& task)
{
    switch (task.operationStatus)
    {
        case math::MathStatus::Ok:
            std::cout << "Operation status: OK" << '\n'
                      << "Result is: " << task.result << '\n';
            break;
        case math::MathStatus::DivideByZero:
            std::cout << "Error: DivideByZero" << '\n';
            break;
        case math::MathStatus::Overflow:
            std::cout << "Error: Overflow" << '\n';
            break;
        case math::MathStatus::FactorialFromNegative:
            std::cout << "Error: FactorialFromNegative" << '\n';
            break;
        case math::MathStatus::ParseError:
            std::cout << "Error: ParseError" << '\n';
            break;
        case math::MathStatus::Help:
            printHelp();
            break;
    }
}

void makeCalculate(utility::Task& task)
{
    if (task.operationStatus != math::MathStatus::Ok)
    {
        return;
    }

    switch (task.operation)
    {
        case '+':
            task.result = math::add(task.firstNumber, task.secondNumber,
                                    task.operationStatus);
            break;
        case '-':
            task.result = math::substract(task.firstNumber, task.secondNumber,
                                          task.operationStatus);
            break;
        case '*':
            task.result = math::multiply(task.firstNumber, task.secondNumber,
                                         task.operationStatus);
            break;
        case '/':
            task.result = math::divide(task.firstNumber, task.secondNumber,
                                       task.operationStatus);
            break;
        case '^':
            task.result = math::power(task.firstNumber, task.secondNumber,
                                      task.operationStatus);
            break;
        case '!':
            task.result =
                math::factorial(task.firstNumber, task.operationStatus);
            break;
        default:
            break;
    }
}

void makeTask(utility::Task& task, const int& argc, char* argv[])
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
                task.operationStatus = math::MathStatus::Help;
                return;
                break;
            case 'f':
                if (!charToInt(optarg, task.firstNumber))
                {
                    task.operationStatus = math::MathStatus::ParseError;
                    return;
                }
                break;
            case 's':
                if (!charToInt(optarg, task.secondNumber))
                {
                    task.operationStatus = math::MathStatus::ParseError;
                    return;
                }
                break;
            case 'o':
                if (!parseOperation(optarg, task.operation))
                {
                    task.operationStatus = math::MathStatus::ParseError;
                    return;
                }
                break;
            case '?':
                break;
            default:
                abort();
        }
    }
}

void applicationRun(utility::Task& task, const int& argc, char* argv[])
{
    makeTask(task, argc, argv);
    makeCalculate(task);
    printResult(task);
}

int main(int argc, char* argv[])
{
    utility::Task task;
    applicationRun(task, argc, argv);
    return 0;
}
