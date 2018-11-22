#ifndef LOGGER_H
#define LOGGER_H

#include <string>
#include <vector>
#include <chrono>
#include <ctime>
#include <iostream>
#include <fstream>
#include <sstream>

#include "listener.h"
#include "singleton.hpp"

enum class FileMode : std::uint8_t
{
    OVERWRITE,
    APPEND,
    NEWFILE,
};

enum class MsgType : std::uint8_t
{
    RAW,        // No effect gray message NO PARSING
    CANONICAL,  // No effect white message
    ITEM,       // Item in list
    TRACK,      // Relative to data/system being tracked NO PARSING
    NOTIFY,     // Relative to an event which should be notified to the user
    SECTION,    // Marks a section
    WARNING,    // Relative to an event which could impact the flow badly
    ERROR,      // Relative to a serious but recoverable error
                    // (eg. missing texture when you have a fall back one)
    FATAL,      // Relative to non recoverable error (eg. out of memory...)
    BANG,       // For code flow analysis
    GOOD,       // For test success
    BAD,        // For test fail
};

enum class LogMode : std::uint8_t
{
    CONSOLE   = 1,
    SCREEN    = 2,
    TEXTFILE  = 4,
    CANONICAL = 5   // Console AND File
};

// Holds a log message together with type and timestamp
struct LogMessage : public WData
{
public:
    typedef std::chrono::high_resolution_clock::time_point TimePoint;
    typedef std::chrono::duration<long, std::ratio<1, 1000000000>> TimeStamp;

    LogMessage(const std::string& message,
               const TimeStamp& timestamp,
               MsgType type=MsgType::CANONICAL,
               LogMode mode=LogMode::CANONICAL)
    : message_(message)
    , mode_(mode)
    , type_(type)
    , timestamp_(timestamp) {}

    LogMessage(std::string&& message,
               const TimeStamp& timestamp,
               MsgType type=MsgType::CANONICAL,
               LogMode mode=LogMode::CANONICAL)
    : message_(std::move(message))
    , mode_(mode)
    , type_(type)
    , timestamp_(timestamp) {}

    std::string message_;
    LogMode     mode_;
    MsgType     type_;

    const TimeStamp timestamp_;
};

class Logger : public Singleton<Logger>, public Listener
{
private:
    FileMode file_mode_;               // What to do with the log file (new, overwrite, append)
    std::vector<LogMessage> messages_; // List of logged messages
    bool widget_scroll_required_;      // When new message logged, widget needs to scroll down
    LogMessage::TimePoint start_time_; // Start time for timestamp handling

    // Singleton boilerplate
    Logger (const Logger&)=delete;
    Logger();
   ~Logger();

    // Print to console with style given a message and its type
    void print_console(const std::string& message, MsgType type, float timestamp=0.f);
    // Replace xml style tags within messages by ANSI escape sequences for styling
    void parse_tags(const std::string& message, std::string& final, MsgType type);
    // Strip xml tags from message
    void strip_tags(const std::string& message, std::string& stripped, MsgType type);

public:
    // Singleton boilerplate
    friend Logger& Singleton<Logger>::Instance();
    friend void Singleton<Logger>::Kill();

    // Actual functions used for logging (functor style)
    void operator ()(const std::string& message,
                     MsgType type=MsgType::CANONICAL,
                     LogMode mode=LogMode::CANONICAL);
    void operator ()(std::string&& message,
                     MsgType type=MsgType::CANONICAL,
                     LogMode mode=LogMode::CANONICAL);
    void operator ()(const LogMessage& log_message);

    template <typename ...Args>
    void printfold(Args&&... args);

    // Logs any event the logger has subscribed to
    void onTrack(const WData& data);
    // Subscribe to an event channel for a given informer
    void track(hash_t chan, Informer& informer);
    // Unsubscribe from a particular event channel
    void untrack(hash_t chan, Informer& informer);
    // Unsubscribe from all channels published by an informer
    void untrack(Informer& informer);

    // Write logged messages to file
    void write(const std::string& log_path);

    // Shows logged message in an ImGui window with filtering options
#ifndef __DISABLE_EDITOR__
    void generate_widget();
#endif

    // Print to console in all existing styles (debug purposes)
    void print_reference();

    // Clear message list
    inline void clear() { messages_.clear(); }
    // Change file mode
    inline void set_mode(FileMode file_mode) { file_mode_ = file_mode; }
};

template <typename ...Args>
void Logger::printfold(Args&&... args)
{
    std::ostringstream out;
    (out << ... << args);
    auto timestamp = std::chrono::high_resolution_clock::now() - start_time_;
    LogMessage log_message(out.str(), timestamp, MsgType::CANONICAL, LogMode::CANONICAL);

    messages_.push_back(log_message);
    float sec = std::chrono::duration_cast<std::chrono::duration<float>>(timestamp).count();
    print_console(log_message.message_, log_message.type_, sec);
}

namespace dbg
{
    static Logger& LOG = Logger::Instance();  //Allows dbg::LOG("message",...) syntax
}

#ifdef __DEBUG__
    #define DLOG_SET(LOG_PATH, ...) do { \
        dbg::LOG.set(LOG_PATH, ##__VA_ARGS__); \
        } while(0)
    #define DLOG_CLEAR() do { \
        dbg::LOG.clear(); \
        } while(0)
    #define DLOG_WRITE() do { \
        dbg::LOG.write(); \
        } while(0)
    #define DLOG_PRINT_REF() do { \
        dbg::LOG.print_reference(); \
        } while(0)
    #define DLOG_TRACK(MTYPESTR, INFORMER) do { \
        dbg::LOG.track(H_(MTYPESTR), INFORMER); \
        } while(0)

    #define FDLOG( ... ) do { \
        dbg::LOG.printfold(__VA_ARGS__); \
        } while(0)
    #define DLOG(MESSAGE, ...) do { \
        dbg::LOG(MESSAGE, ##__VA_ARGS__); \
        } while(0)
    #define DLOGR(MESSAGE) do { \
        dbg::LOG(MESSAGE, MsgType::RAW); \
        } while(0)
    #define DLOGI(MESSAGE) do { \
        dbg::LOG(MESSAGE, MsgType::ITEM); \
        } while(0)
    #define DLOGT(MESSAGE) do { \
        dbg::LOG(MESSAGE, MsgType::TRACK); \
        } while(0)
    #define DLOGN(MESSAGE) do { \
        dbg::LOG(MESSAGE, MsgType::NOTIFY); \
        } while(0)
    #define DLOGS(MESSAGE) do { \
        dbg::LOG(MESSAGE, MsgType::SECTION); \
        } while(0)
    #define DLOGW(MESSAGE) do { \
        dbg::LOG(MESSAGE, MsgType::WARNING); \
        } while(0)
    #define DLOGE(MESSAGE) do { \
        dbg::LOG(MESSAGE, MsgType::ERROR); \
        } while(0)
    #define DLOGF(MESSAGE) do { \
        dbg::LOG(MESSAGE, MsgType::FATAL); \
        } while(0)
    #define DLOGG(MESSAGE) do { \
        dbg::LOG(MESSAGE, MsgType::GOOD); \
        } while(0)
    #define DLOGB(MESSAGE) do { \
        dbg::LOG(MESSAGE, MsgType::BAD); \
        } while(0)
    #define BANG() do { \
        dbg::LOG(std::string(__FILE__)+":"+std::to_string(__LINE__), MsgType::BANG); \
        } while(0)
#else
    #define DLOG_SET(LOG_PATH, ...)     do { } while(0)
    #define DLOG_CLEAR()                do { } while(0)
    #define DLOG_WRITE()                do { } while(0)
    #define DLOG_PRINT_REF()            do { } while(0)
    #define DLOG_TRACK(MTYPE, INFORMER) do { } while(0)
    #define DLOG(MESSAGE, ...)          do { } while(0)
    #define FDLOG( ... )                do { } while(0)
    #define DLOGR(MESSAGE)              do { } while(0)
    #define DLOGI(MESSAGE)              do { } while(0)
    #define DLOGT(MESSAGE)              do { } while(0)
    #define DLOGN(MESSAGE)              do { } while(0)
    #define DLOGS(MESSAGE)              do { } while(0)
    #define DLOGW(MESSAGE)              do { } while(0)
    #define DLOGE(MESSAGE)              do { } while(0)
    #define DLOGF(MESSAGE)              do { } while(0)
    #define DLOGG(MESSAGE)              do { } while(0)
    #define DLOGB(MESSAGE)              do { } while(0)
    #define BANG(MESSAGE)               do { } while(0)
#endif

#endif // LOGGER_H
