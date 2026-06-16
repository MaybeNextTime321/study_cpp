#include <customMath.h>
#include <getopt.h>
#include <unistd.h>

#include <cstdio>
#include <cstdlib>

#include "cmath"

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
                printf( // NOLINT(cppcoreguidelines-pro-type-vararg)
                    "Operation status: OK\n");
                printf( // NOLINT(cppcoreguidelines-pro-type-vararg)
                    "Result is: %f\n", task_.result);
                break;
            case math::MathStatus::DivideByZero:
                printf( // NOLINT(cppcoreguidelines-pro-type-vararg)
                    "Error: DivideByZero\n");
                break;
            case math::MathStatus::Overflow:
                printf( // NOLINT(cppcoreguidelines-pro-type-vararg)
                    "Error: Overflow\n");
                break;
            case math::MathStatus::FactorialFromNegative:
                printf( // NOLINT(cppcoreguidelines-pro-type-vararg)
                    "Error: FactorialFromNegative\n");
                break;
            case math::MathStatus::ParseError:
                printf( // NOLINT(cppcoreguidelines-pro-type-vararg)
                    "Error: ParseError\n");
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
            printf( // NOLINT(cppcoreguidelines-pro-type-vararg)
                "Error while parsing par: %s\n", optarg);
            return false;
        }

        variableToWrite = static_cast<int>(parsed);
        printf("Parsed par: %i \n", // NOLINT(cppcoreguidelines-pro-type-vararg)
               variableToWrite);
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
                printf( // NOLINT(cppcoreguidelines-pro-type-vararg)
                    "Parsed op: %c\n", operation);
                task_.operation = operation;
                return true;
            default:
                printf( // NOLINT(cppcoreguidelines-pro-type-vararg)
                    "Error while parsing op:%c\n", operation);
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
                    if (!charToInt(optarg, task_.firstNumber))
                    {
                        task_.operationStatus = math::MathStatus::ParseError;
                        return;
                    }
                    break;
                case 's':
                    if (!charToInt(optarg, task_.secondNumber))
                    {
                        task_.operationStatus = math::MathStatus::ParseError;
                        return;
                    }
                    break;
                case 'o':
                    if (!parseOperation(optarg))
                    {
                        task_.operationStatus = math::MathStatus::ParseError;
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

    void printHelp() // NOLINT(readability-convert-member-functions-to-static)
        const
    {
        printf( // NOLINT(cppcoreguidelines-pro-type-vararg)
            "Usage my calculation  [OPTIONS]:\n");
        printf("Options:\n"); // NOLINT(cppcoreguidelines-pro-type-vararg)
        printf(               // NOLINT(cppcoreguidelines-pro-type-vararg)
            "  --firstNumber,  -f  First number\n");
        printf( // NOLINT(cppcoreguidelines-pro-type-vararg)
            "  --secondNumber, -s  Second number\n");
        printf( // NOLINT(cppcoreguidelines-pro-type-vararg)
            "  --operation,    -o  Operation (+, -, *, /, ^, !, %%)\n");
        printf( // NOLINT(cppcoreguidelines-pro-type-vararg)
            "  --help,         -h  Show this help\n");
        printf("Example:\n");    // NOLINT(cppcoreguidelines-pro-type-vararg)
        printf("  calculation"); // NOLINT(cppcoreguidelines-pro-type-vararg)
        printf(                  // NOLINT(cppcoreguidelines-pro-type-vararg)
            " --firstNumber 5 --secondNumber 3 --operation +\n");
        printf( // NOLINT(cppcoreguidelines-pro-type-vararg)
            "  Note: use quotes for * operator: --operation '*'\n");
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