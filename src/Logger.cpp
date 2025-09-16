#include <Logger.h>

// Set default value of _level
LogLevel Logger::_level = LogLevel::Info;

// Constructor
Logger::Logger() {}

// Public Functions
void Logger::Log(const std::string &message) /*Assume the log level to be none*/
{
  std::cout << GREEN_COLOR << "[] " << RESET_COLOR << message;
}

void Logger::Log(LogLevel level, const std::string &message) {
  if (level < _level)
    return;
  switch (level) {
  case Info:
    LogInfo(message);
    break;
  case Warning:
    LogWarning(message);
    break;
  case Error:
    LogError(message);
    break;
  case None:
    std::cout << RESET_COLOR << message;
    break;
  default:
    break;
  }
}

void Logger::fmtLog(LogLevel level, const char *const message, ...) {
  if (level < _level)
    return;

  va_list args;
  va_start(args, message);
  switch (level) {
  case Info:
    std::cout << BLUE_COLOR << "[INFO]: " << RESET_COLOR;
    vprintf(message, args);

    break;
  case Warning:
    std::cout << YELLOW_COLOR << "[WARN]: " << RESET_COLOR;
    vprintf(message, args);
    break;
  case Error:
    std::cout << RED_COLOR << "[ERROR]: " << RESET_COLOR;
    vprintf(message, args);
    break;
  case None:

    vprintf(message, args);
    break;
  }
  va_end(args);
  printf("\n");
}

void Logger::fmtLog(const char *message,
                    ...) /* Assume the log level to be none */
{
  va_list args;
  va_start(args, message);
  vprintf(message, args);
  va_end(args);

  printf("\n");
}

void Logger::SetLogLevel(LogLevel level) { _level = level; }

LogLevel Logger::GetLogLevel() { return _level; }

// Private Functions
void Logger::LogInfo(const std::string &message) {
  std::cout << BLUE_COLOR << "[INFO]: " << RESET_COLOR << message << std::endl;
}

void Logger::LogWarning(const std::string &message) {
  std::cout << YELLOW_COLOR << "[WARN]: " << RESET_COLOR << message
            << std::endl;
}

void Logger::LogError(const std::string &message) {
  std::cout << RED_COLOR << "[ERROR]: " << RESET_COLOR << message << std::endl;
}
