#pragma once
#include "spdlog/spdlog.h"

#include <customMath.h>
#include <getopt.h>
#include <libpq-fe.h>
#include <unistd.h>

#include <nlohmann/json.hpp>

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <unordered_map>

#include "cmath"

namespace utility
{
class Logger
{
  private:
    Logger() = default;
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;

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
class DataBase
{
  public:
    DataBase()
    {
        conn = PQconnectdb(conninfo);

        /* Check to see that the backend connection was successfully made */
        if (PQstatus(conn) != CONNECTION_OK)
        {
            utility::Logger::getInstance().log(
                std::string("Error while connect to Data Base " +
                            std::string(PQerrorMessage(conn))),
                utility::Logger::LogLevel::CRITICAL);
            return;
        }

        res = PQexec(conn,
                     "SELECT pg_catalog.set_config('search_path', '', false)");
        if (PQresultStatus(res) != PGRES_TUPLES_OK)
        {
            utility::Logger::getInstance().log(
                std::string("Data Base SET failed: " +
                            std::string(PQerrorMessage(conn))),
                utility::Logger::LogLevel::CRITICAL);
            PQclear(res);
            return;
        }

        PQclear(res);
    }
    ~DataBase()
    {
        PQfinish(conn);
    }

    DataBase& operator=(const DataBase& other) = delete;
    DataBase(const DataBase& other) = delete;

    DataBase& operator=(
        DataBase&& other) // NOLINT(readability-redundant-access-specifiers)
    {
        if (this != &other)
        {
            conn = other.conn;
            res = other.res;
            notify = other.notify;
            nnotifies = other.nnotifies;
        }
        return *this;
    }

    DataBase(DataBase&& other)
    {
        if (this != &other)
        {
            conn = other.conn;
            res = other.res;
            notify = other.notify;
            nnotifies = other.nnotifies;
        }
    }

    void WriteTask(int firstNumber, int secondNumber, char operation,
                   double result) // NOLINT(unused-parameter)
    {
        std::string connectionValue = "INSERT INTO public.calculation "
                                      "(operation, firstNumber, secondNumber, "
                                      "result) VALUES ('" +
                                      std::string(1, operation) + "\', " +
                                      std::to_string(firstNumber) + ", " +
                                      std::to_string(secondNumber) + ", " +
                                      std::to_string(result) + ");";

        res = PQexec(conn, connectionValue.c_str());
        if (PQresultStatus(res) != PGRES_COMMAND_OK)
        {
            utility::Logger::getInstance().log(
                std::string("Eroor while write in Data Base: " +
                            std::string(PQerrorMessage(conn))),
                utility::Logger::LogLevel::CRITICAL);
            PQclear(res);
            return;
        }
        PQclear(res);
    }

    void GetOperations(std::unordered_map<std::string, int>& operations)
    {
        res = PQexec(conn, "SELECT * FROM public.calculation");

        if (PQresultStatus(res) != PGRES_TUPLES_OK)
        {
            return;
        }

        utility::Logger::getInstance().log(
            std::string("Start Loading Cash data: "),
            utility::Logger::LogLevel::INFO);
        size_t rows = PQntuples(res);

        utility::Logger::getInstance().log(
            std::string("Founded " + std::to_string(rows) + " records"),
            utility::Logger::LogLevel::INFO);

        for (size_t i = 0; i < rows; i++)
        {
            char operation = PQgetvalue(res, i, 0)[0];
            int firstNumber = std::atoi(PQgetvalue(res, i, 1));
            int secondNumber = std::atoi(PQgetvalue(res, i, 2));
            double result = std::atof(PQgetvalue(res, i, 3));

            utility::Logger::getInstance().log(
                std::string("Loaded hew item FN: ") +
                    std::to_string(firstNumber) +
                    ", SN: " + std::to_string(secondNumber) +
                    +" Op: " + operation + " Res: " + std::to_string(result),
                utility::Logger::LogLevel::INFO);

            std::string key = std::to_string(firstNumber) + operation +
                              std::to_string(secondNumber);

            if (operations.find(key) == operations.end())
            {
                operations.emplace(key, result);
            }
        }

        PQclear(res);
    }

  private:
    const char* conninfo{"host=localhost port=5432 dbname=postgres "
                         "connect_timeout=10 user=postgres password=1466"};
    PGconn* conn{nullptr};
    PGresult* res{nullptr};
    PGnotify* notify{nullptr};
    int nnotifies{0};
};

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
        loadCashe();
        makeCalculate();
        printResult();
    }

    struct Task
    {
        int firstNumber{0};
        int secondNumber{0};
        char operation{0};
        double result{0.0};
        math::MathStatus operationStatus{math::MathStatus::Ok};
    };

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

    void loadCashe()
    {
        if (task_.operationStatus != math::MathStatus::Ok)
        {
            return;
        }

        cash_.clear();
        dataBase_.GetOperations(cash_);
    }

    void storeTask(const std::string& taskKey)
    {
        dataBase_.WriteTask(task_.firstNumber, task_.secondNumber,
                            task_.operation, task_.result);
        cash_.emplace(taskKey, task_.result);
    }

    void makeCalculate()
    {
        if (task_.operationStatus != math::MathStatus::Ok)
        {
            return;
        }

        const std::string taskKey = std::to_string(task_.firstNumber) +
                                    task_.operation +
                                    std::to_string(task_.secondNumber);

        if (cash_.find(taskKey) == cash_.end())
        {
            switch (task_.operation)
            {
                case '+':
                    task_.result =
                        math::add(task_.firstNumber, task_.secondNumber,
                                  task_.operationStatus);
                    storeTask(taskKey);
                    break;
                case '-':
                    task_.result =
                        math::substract(task_.firstNumber, task_.secondNumber,
                                        task_.operationStatus);
                    storeTask(taskKey);
                    break;
                case '*':
                    task_.result =
                        math::multiply(task_.firstNumber, task_.secondNumber,
                                       task_.operationStatus);
                    storeTask(taskKey);
                    break;
                case '/':
                    task_.result =
                        math::divide(task_.firstNumber, task_.secondNumber,
                                     task_.operationStatus);
                    storeTask(taskKey);
                    break;
                case '^':
                    task_.result =
                        math::power(task_.firstNumber, task_.secondNumber,
                                    task_.operationStatus);
                    storeTask(taskKey);
                    break;
                case '!':
                    task_.result = math::factorial(task_.firstNumber,
                                                   task_.operationStatus);
                    storeTask(taskKey);
                    break;
                default:
                    break;
            }
        }
        else
        {
            task_.result = cash_.find(taskKey)->second;
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

        if (task_.operation == '*' || task_.operation == '+')
        {
            const int minValue =
                std::min(task_.firstNumber, task_.secondNumber);
            const int maxValue =
                std::max(task_.firstNumber, task_.secondNumber);

            task_.firstNumber = minValue;
            task_.secondNumber = maxValue;
        }

        if (task_.operation == '!')
        {
            task_.secondNumber = 0;
        }
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

  private: // NOLINT(readability-redundant-access-specifiers)
    Task task_;
    DataBase dataBase_;
    std::unordered_map<std::string, int> cash_;
};

} // namespace calculator