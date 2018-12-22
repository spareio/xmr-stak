#include "Logger.h"

#include <boost/filesystem/operations.hpp>

Logger::~Logger()
{
    m_file.close();
}

Logger& Logger::getInstance()
{
    static Logger instance;
    return instance;
}

void Logger::setFileName(const std::string fileName)
{
    m_fileName = fileName;
    m_toFile = !m_fileName.empty();
    if (m_toFile)
    {
        if (exists(m_fileName))
        {
            remove(m_fileName);
        }
        m_file.open(m_fileName, ofstream::out | ofstream::app);
    }
}

std::string Logger::getFileName() const
{
    return m_fileName;
}

void Logger::setVerboseLevel(VerboseLevel verboseLevel)
{
    m_verboseLevel = verboseLevel;
}

Logger::VerboseLevel Logger::getVerboseLevel() const
{
    return m_verboseLevel;
}

std::string Logger::getVerboseLevelMessage(VerboseLevel l)
{
    static_assert(Logger::E_VERBOSE_LEVEL__END == 3, "Need to implement method for new event");

    switch (l)
    {
    case E_DEBUG:
        return "DEBUG:    ";
    case E_INFO:
        return "INFO:     ";
    case E_WARNING:
        return "WARNING:  ";
    case E_CRITICAL:
        return "CRITICAL: ";
    default:
        return "Not implemented level";
    }
}

std::istream& operator>>(std::istream& in, Logger::VerboseLevel& verboseLevel)
{
    static_assert(Logger::E_VERBOSE_LEVEL__END == 3, "Need to implement operator for new event");

    std::string token;
    in >> token;
    if (token == "0")
        verboseLevel = Logger::VerboseLevel::E_DEBUG;
    else if (token == "1")
        verboseLevel = Logger::VerboseLevel::E_INFO;
    else if (token == "2")
        verboseLevel = Logger::VerboseLevel::E_WARNING;
    else if (token == "3")
        verboseLevel = Logger::VerboseLevel::E_CRITICAL;
    else
        in.setstate(std::ios_base::failbit);
    return in;
}
