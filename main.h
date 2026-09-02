#pragma once
#include "spdlog/spdlog.h"

#include <customMath.h>
#include <getopt.h>
#include <libpq-fe.h>
#include <unistd.h>

#include <boost/asio.hpp>
#include <nlohmann/json.hpp>

#include <atomic>
#include <chrono>
#include <csignal>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <stdexcept>
#include <thread>
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
        conn.reset(PQconnectdb(conninfo.c_str()));

        if (!conn)
        {
            utility::Logger::getInstance().log(
                std::string("Error while connect to Data Base "),
                utility::Logger::LogLevel::CRITICAL);
            return;
        }

        /* Check to see that the backend connection was successfully made */
        if (PQstatus(conn.get()) != CONNECTION_OK)
        {
            utility::Logger::getInstance().log(
                std::string("Error while connect to Data Base " +
                            std::string(PQerrorMessage(conn.get()))),
                utility::Logger::LogLevel::CRITICAL);
            return;
        }

        res.reset(
            PQexec(conn.get(),
                   "SELECT pg_catalog.set_config('search_path', '', false)"));
        if (PQresultStatus(res.get()) != PGRES_TUPLES_OK)
        {
            utility::Logger::getInstance().log(
                std::string("Data Base SET failed: " +
                            std::string(PQerrorMessage(conn.get()))),
                utility::Logger::LogLevel::CRITICAL);
            return;
        }
    }

    void WriteTask(int firstNumber, int secondNumber, char operation,
                   double result,
                   int operationStatus) // NOLINT(unused-parameter)
    {
        if (!conn)
        {
            utility::Logger::getInstance().log(
                std::string("Error while connect to Data Base "),
                utility::Logger::LogLevel::CRITICAL);
            return;
        }

        const char* command = "INSERT INTO public.calculation "
                              "(operation, firstNumber, secondNumber, "
                              "result, operationstatus) VALUES "
                              "($1, $2, $3, $4, $5)";

        const std::string operationParam(1, operation);
        const std::string firstNumberParam = std::to_string(firstNumber);
        const std::string secondNumberParam = std::to_string(secondNumber);
        const std::string resultParam = std::to_string(result);
        const std::string operationStatusParam =
            std::to_string(operationStatus);

        const char* paramValues[] = {
            operationParam.c_str(), firstNumberParam.c_str(),
            secondNumberParam.c_str(), resultParam.c_str(),
            operationStatusParam.c_str()};

        res.reset(PQexecParams(conn.get(), command, 5, nullptr, paramValues,
                               nullptr, nullptr, 0));
        if (PQresultStatus(res.get()) != PGRES_COMMAND_OK)
        {
            utility::Logger::getInstance().log(
                std::string("Eroor while write in Data Base: " +
                            std::string(PQerrorMessage(conn.get()))),
                utility::Logger::LogLevel::CRITICAL);
            return;
        }
    }

    void GetOperations(
        std::unordered_map<std::string, std::pair<int, int>>& operations)
    {
        if (!conn)
        {
            utility::Logger::getInstance().log(
                std::string("Error while connect to Data Base "),
                utility::Logger::LogLevel::CRITICAL);
            return;
        }
        res.reset(PQexec(conn.get(), "SELECT * FROM public.calculation"));

        if (res == nullptr || PQresultStatus(res.get()) != PGRES_TUPLES_OK)
        {
            utility::Logger::getInstance().log(
                std::string("Error While Get Operation from Data Base"),
                utility::Logger::LogLevel::CRITICAL);
            return;
        }

        utility::Logger::getInstance().log(
            std::string("Start Loading Cash data: "),
            utility::Logger::LogLevel::INFO);
        size_t rows = PQntuples(res.get());

        utility::Logger::getInstance().log(
            std::string("Founded " + std::to_string(rows) + " records"),
            utility::Logger::LogLevel::INFO);

        for (size_t i = 0; i < rows; i++)
        {
            char operation = PQgetvalue(res.get(), i, 0)[0];
            int firstNumber = std::atoi(PQgetvalue(res.get(), i, 1));
            int secondNumber = std::atoi(PQgetvalue(res.get(), i, 2));
            double result = std::atof(PQgetvalue(res.get(), i, 3));
            int operationStatus = std::atoi(PQgetvalue(res.get(), i, 4));

            utility::Logger::getInstance().log(
                std::string("Loaded hew item FN: ") +
                    std::to_string(firstNumber) +
                    ", SN: " + std::to_string(secondNumber) +
                    +" Op: " + operation + " Res: " + std::to_string(result) +
                    +" Status: " + std::to_string(operationStatus),
                utility::Logger::LogLevel::INFO);

            std::string key = std::to_string(firstNumber) + operation +
                              std::to_string(secondNumber);

            if (operations.find(key) == operations.end())
            {
                operations.emplace(
                    key, std::pair<int, int>(result, operationStatus));
            }
        }
    }

  private:
    const std::string conninfo{"host=127.0.0.1 port=5432 dbname=postgres "
                               "connect_timeout=1 user=postgres password=1466"};

    struct PGconnDeleter
    {
        void operator()(PGconn* conn) const
        {
            if (conn)
                PQfinish(conn);
        }
    };

    struct PGresultDeleter
    {
        void operator()(PGresult* res) const
        {
            if (res)
                PQclear(res);
        }
    };

    std::unique_ptr<PGconn, PGconnDeleter> conn;
    std::unique_ptr<PGresult, PGresultDeleter> res;
};

class Application
{
  public:
    struct CalculationResult
    {
        math::MathStatus status{math::MathStatus::Ok};
        double result{0.0};
    };

    Application() : task_(Task())
    {
        signal_handler_installed_.store(false, std::memory_order_release);
    };

    ~Application() noexcept
    {
        StopSignalHandling();
    }

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

    bool IsShutdownRequested() const
    {
        return shutdown_requested_.load(std::memory_order_acquire);
    }

    void StartSignalHandling()
    {
        if (signal_thread_started_.exchange(true, std::memory_order_acq_rel))
        {
            return;
        }

        shutdown_requested_.store(false, std::memory_order_release);
        signal_thread_stop_.store(false, std::memory_order_release);

        struct sigaction action
        {};
        action.sa_handler = &HandleTerminationSignal;
        sigemptyset(&action.sa_mask);
        action.sa_flags = 0;

        if (sigaction(SIGTERM, &action, nullptr) != 0)
        {
            signal_thread_started_.store(false, std::memory_order_release);
            utility::Logger::getInstance().log(
                "Failed to install SIGTERM handler",
                utility::Logger::LogLevel::ERROR);
            return;
        }

        signal_handler_installed_.store(true, std::memory_order_release);

        signal_monitor_thread_ = std::thread([this]() {
            while (!signal_thread_stop_.load(std::memory_order_acquire))
            {
                if (shutdown_requested_.load(std::memory_order_acquire))
                {
                    utility::Logger::getInstance().log(
                        "Shutdown requested by SIGTERM",
                        utility::Logger::LogLevel::WARN);
                    return;
                }
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
            }
        });
    }

    void StopSignalHandling()
    {
        signal_thread_stop_.store(true, std::memory_order_release);

        if (signal_monitor_thread_.joinable())
        {
            signal_monitor_thread_.join();
        }

        if (signal_handler_installed_.exchange(false,
                                               std::memory_order_acq_rel))
        {
            struct sigaction action
            {};
            action.sa_handler = SIG_DFL;
            sigemptyset(&action.sa_mask);
            action.sa_flags = 0;
            sigaction(SIGTERM, &action, nullptr);
        }

        signal_thread_started_.store(false, std::memory_order_release);
    }

    CalculationResult EvaluateJsonRequest(const std::string& jsonText)
    {
        Task localTask = createTaskFromJson(jsonText);
        if (localTask.operationStatus != math::MathStatus::Ok)
        {
            return {localTask.operationStatus, 0.0};
        }

        const std::string taskKey = std::to_string(localTask.firstNumber) +
                                    localTask.operation +
                                    std::to_string(localTask.secondNumber);

        if (cash_.find(taskKey) == cash_.end())
        {
            switch (localTask.operation)
            {
                case '+':
                    localTask.result =
                        math::add(localTask.firstNumber, localTask.secondNumber,
                                  localTask.operationStatus);
                    break;
                case '-':
                    localTask.result = math::substract(
                        localTask.firstNumber, localTask.secondNumber,
                        localTask.operationStatus);
                    break;
                case '*':
                    localTask.result = math::multiply(
                        localTask.firstNumber, localTask.secondNumber,
                        localTask.operationStatus);
                    break;
                case '/':
                    localTask.result = math::divide(localTask.firstNumber,
                                                    localTask.secondNumber,
                                                    localTask.operationStatus);
                    break;
                case '^':
                    localTask.result = math::power(localTask.firstNumber,
                                                   localTask.secondNumber,
                                                   localTask.operationStatus);
                    break;
                case '!':
                    localTask.result = math::factorial(
                        localTask.firstNumber, localTask.operationStatus);
                    break;
                default:
                    localTask.operationStatus = math::MathStatus::ParseError;
                    break;
            }
        }
        else
        {
            const std::pair<int, int> record = cash_.find(taskKey)->second;
            localTask.result = record.first;
            localTask.operationStatus =
                static_cast<math::MathStatus>(record.second);
        }

        return {localTask.operationStatus, localTask.result};
    }

    void run(int argc, char** argv)
    {
        StartSignalHandling();

        if (IsShutdownRequested())
        {
            StopSignalHandling();
            return;
        }

        makeTask(argc, argv);
        if (IsShutdownRequested())
        {
            StopSignalHandling();
            return;
        }

        loadCashe();
        if (IsShutdownRequested())
        {
            StopSignalHandling();
            return;
        }

        makeCalculate();
        if (IsShutdownRequested())
        {
            StopSignalHandling();
            return;
        }

        printResult();
        StopSignalHandling();
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
                              const std::string& key, Task& task)
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
            task.operationStatus = math::MathStatus::ParseError;
        }
    }

    Task parseTaskFromJson(const std::string& jsonText)
    {
        Task parsedTask{};
        nlohmann::json jsonInput;

        try
        {
            jsonInput = nlohmann::json::parse(jsonText);
        }
        catch (...)
        {
            utility::Logger::getInstance().log(
                std::string("Error while loading JSON. Please check you input"),
                utility::Logger::LogLevel::CRITICAL);
            parsedTask.operationStatus = math::MathStatus::ParseError;
            return parsedTask;
        }

        parseVariableToValue(jsonInput, parsedTask.firstNumber, "firstNumber",
                             parsedTask);
        parseVariableToValue(jsonInput, parsedTask.secondNumber, "secondNumber",
                             parsedTask);
        std::string operationString{};
        parseVariableToValue(jsonInput, operationString, "operation",
                             parsedTask);
        parseOperationForTask(parsedTask, operationString.c_str());

        if (parsedTask.operation == '*' || parsedTask.operation == '+')
        {
            const int minValue =
                std::min(parsedTask.firstNumber, parsedTask.secondNumber);
            const int maxValue =
                std::max(parsedTask.firstNumber, parsedTask.secondNumber);

            parsedTask.firstNumber = minValue;
            parsedTask.secondNumber = maxValue;
        }

        if (parsedTask.operation == '!')
        {
            parsedTask.secondNumber = 0;
        }

        return parsedTask;
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
                            task_.operation, task_.result,
                            static_cast<int>(task_.operationStatus));
        cash_.emplace(taskKey, std::pair<int, int>(
                                   task_.result,
                                   static_cast<int>(task_.operationStatus)));
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
            const std::pair<int, int> record = cash_.find(taskKey)->second;
            task_.result = record.first;
            task_.operationStatus =
                static_cast<math::MathStatus>(record.second);
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
        return parseOperationForTask(task_, optarg);
    }

    bool parseOperationForTask(Task& targetTask, const char* optarg)
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
                targetTask.operation = operation;
                return true;
            default:
                utility::Logger::getInstance().log(
                    std::string("Operator is empty or couldn't be parsed"),
                    utility::Logger::LogLevel::CRITICAL);
                targetTask.operationStatus = math::MathStatus::ParseError;
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

        task_ = parseTaskFromJson(argv[1]);
    }

    Task createTaskFromJson(const std::string& jsonText)
    {
        return parseTaskFromJson(jsonText);
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

  private:
    static void HandleTerminationSignal(int signal_number)
    {
        if (signal_number != SIGTERM)
        {
            return;
        }

        shutdown_requested_.store(true, std::memory_order_release);
    }

    static inline std::atomic<bool> shutdown_requested_{false};
    static inline std::atomic<bool> signal_handler_installed_{false};
    static inline std::atomic<bool> signal_thread_started_{false};
    static inline std::atomic<bool> signal_thread_stop_{false};

    Task task_;
    DataBase dataBase_;
    std::unordered_map<std::string, std::pair<int, int>> cash_;
    std::thread signal_monitor_thread_;
};

class NetworkServer
{
  public:
    NetworkServer(const std::string& host, unsigned short port) :
        host_(host), port_(port),
        acceptor_(io_context_, boost::asio::ip::tcp::endpoint(
                                   boost::asio::ip::make_address(host), port))
    {
        port_ = acceptor_.local_endpoint().port();
    }

    ~NetworkServer()
    {
        Stop();
    }

    void Start()
    {
        if (accepting_.exchange(true, std::memory_order_acq_rel))
        {
            return;
        }

        acceptor_.listen();
        port_ = acceptor_.local_endpoint().port();
        acceptor_.non_blocking(true);
        accept_thread_ = std::thread([this]() {
            while (accepting_.load(std::memory_order_acquire))
            {
                try
                {
                    auto client_socket =
                        std::make_unique<boost::asio::ip::tcp::socket>(
                            io_context_);
                    boost::system::error_code accept_error;
                    acceptor_.accept(*client_socket, accept_error);

                    if (accept_error == boost::asio::error::would_block ||
                        accept_error == boost::asio::error::try_again)
                    {
                        std::this_thread::yield();
                        continue;
                    }
                    if (accept_error)
                    {
                        if (!accepting_.load(std::memory_order_acquire))
                        {
                            break;
                        }
                        continue;
                    }

                    if (!accepting_.load(std::memory_order_acquire))
                    {
                        break;
                    }

                    std::string request = ReadMessage(*client_socket);
                    Application application;
                    const Application::CalculationResult result =
                        application.EvaluateJsonRequest(request);
                    const std::string response = BuildResponse(result);
                    WriteMessage(*client_socket, response);
                    client_socket->close();
                }
                catch (const boost::system::system_error&)
                {
                    if (!accepting_.load(std::memory_order_acquire))
                    {
                        break;
                    }
                }
                catch (...)
                {
                    if (!accepting_.load(std::memory_order_acquire))
                    {
                        break;
                    }
                }
            }
        });
    }

    void Stop()
    {
        accepting_.store(false, std::memory_order_release);

        if (accept_thread_.joinable())
        {
            accept_thread_.join();
        }

        boost::system::error_code ignored;
        acceptor_.cancel(ignored);
        acceptor_.close(ignored);
    }

    unsigned short GetPort() const
    {
        return port_;
    }

  private:
    static std::string
        BuildResponse(const Application::CalculationResult& result)
    {
        nlohmann::json responseJson;
        responseJson["status"] =
            result.status == math::MathStatus::Ok ? "OK" : "ERROR";
        responseJson["result"] = result.result;
        responseJson["error"] =
            result.status == math::MathStatus::Ok ? "" : "operation_error";
        return responseJson.dump();
    }

    static std::string ReadMessage(boost::asio::ip::tcp::socket& socket)
    {
        boost::asio::streambuf buffer;
        boost::asio::read_until(socket, buffer, '\n');

        std::istream input_stream(&buffer);
        std::string message;
        std::getline(input_stream, message);

        if (!message.empty() && message.back() == '\r')
        {
            message.pop_back();
        }

        return message;
    }

    static void WriteMessage(boost::asio::ip::tcp::socket& socket,
                             const std::string& message)
    {
        const std::string frame = message + "\n";
        boost::asio::write(socket, boost::asio::buffer(frame));
    }

    std::string host_;
    unsigned short port_;
    boost::asio::io_context io_context_{};
    boost::asio::ip::tcp::acceptor acceptor_;
    std::atomic<bool> accepting_{false};
    std::thread accept_thread_;
};

class NetworkClient
{
  public:
    NetworkClient(const std::string& host, unsigned short port) :
        host_(host), port_(port)
    {}

    bool SendRequest(const std::string& request, std::string& response,
                     std::chrono::milliseconds timeout)
    {
        try
        {
            boost::asio::io_context io_context;
            boost::asio::ip::tcp::socket socket(io_context);
            boost::asio::ip::tcp::resolver resolver(io_context);
            boost::asio::connect(
                socket, resolver.resolve(host_, std::to_string(port_)));

            std::atomic<bool> completed{false};
            std::thread timeoutThread([&]() {
                std::this_thread::sleep_for(timeout);
                if (!completed.load(std::memory_order_acquire))
                {
                    socket.cancel();
                }
            });

            try
            {
                boost::asio::write(socket, boost::asio::buffer(request + "\n"));
                boost::asio::streambuf response_buffer;
                boost::asio::read_until(socket, response_buffer, '\n');

                std::istream input_stream(&response_buffer);
                std::string payload;
                std::getline(input_stream, payload);
                if (!payload.empty() && payload.back() == '\r')
                {
                    payload.pop_back();
                }

                completed.store(true, std::memory_order_release);
                timeoutThread.join();
                response = payload;
                socket.close();
                return true;
            }
            catch (...)
            {
                completed.store(true, std::memory_order_release);
                timeoutThread.join();
                throw;
            }
        }
        catch (const boost::system::system_error&)
        {
            return false;
        }
        catch (...)
        {
            return false;
        }
    }

  private:
    std::string host_;
    unsigned short port_;
};

} // namespace calculator