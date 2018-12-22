#pragma once

#include "spareio-core/TimeZone.h"
#include "spareio-core/TelemetryHelper.h"

#include <string>
#include <iostream>

#include <boost/filesystem/fstream.hpp>

using namespace boost::filesystem;

class Logger
{
public:
    enum VerboseLevel
    {
        E_VERBOSE_LEVEL__BEGIN = 0,
            E_DEBUG = E_VERBOSE_LEVEL__BEGIN,
            E_INFO,
            E_WARNING,
            E_CRITICAL,
        E_VERBOSE_LEVEL__END = E_CRITICAL
    };

    virtual ~Logger();

    static Logger& getInstance();

    void setFileName(const std::string fileName);
    std::string getFileName() const;

    void setVerboseLevel(VerboseLevel verboseLevel);
    VerboseLevel getVerboseLevel() const;

    inline Logger& operator<< (TelemetryHelper::Event event) 
    {
        getInstance() << TelemetryHelper::getStandardMessage(event);
        return *this;
    }

    inline Logger& operator<< (VerboseLevel level) {
        m_currentMessageVerboseLevel = level;
        if (m_currentMessageVerboseLevel >= m_verboseLevel)
        {
            if (m_toFile)
            {
                m_file << TimeZone::getUtcDateTimeSimple() << "  " << getVerboseLevelMessage(level);
            }
            else
            {
                std::cout << TimeZone::getUtcDateTimeSimple() << "  " << getVerboseLevelMessage(level);
            }
        }
        return *this;
    }

    template <typename T>
    inline Logger& operator<< (const T& x) {
        if (m_currentMessageVerboseLevel >= m_verboseLevel)
        {
            if (m_toFile)
            {
                m_file << x;
            }
            else
            {
                std::cout << x; 
            }
        }
        return *this;
    }

    typedef Logger& (*LoggerManipulator)(Logger&);
    Logger& operator<<(LoggerManipulator manip)
    {
        return manip(*this);
    }

    static Logger& endl(Logger& logger)
    {
        if (logger.m_currentMessageVerboseLevel >= logger.m_verboseLevel)
        {
            if (logger.m_toFile)
            {
                logger.m_file << std::endl;
            }
            else
            {
                std::cout << std::endl;
            }
        }
        return logger;
    }

    typedef std::basic_ostream<char, std::char_traits<char> > CoutType;
    typedef CoutType& (*StandardEndLine)(CoutType&);

    Logger& operator<<(StandardEndLine manip)
    {
        if (m_currentMessageVerboseLevel >= m_verboseLevel)
        {
            m_currentMessageVerboseLevel = m_verboseLevel;
            manip(std::cout);
        }
        return *this;
    }

private:
    Logger() {};
    Logger(const Logger& other) = delete;
    Logger& operator=(const Logger&) = delete;

    static std::string getVerboseLevelMessage(VerboseLevel l);

    bool m_toFile{ false };
    std::string m_fileName;
    ofstream m_file;
    VerboseLevel m_verboseLevel{ VerboseLevel::E_WARNING };
    VerboseLevel m_currentMessageVerboseLevel{ VerboseLevel::E_WARNING };
};

std::istream& operator>>(std::istream& in, Logger::VerboseLevel& verboseLevel);
