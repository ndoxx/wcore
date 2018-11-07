#ifndef LOGGER_H
#define LOGGER_H

#include <string>
#include <vector>
#include <chrono>
#include <ctime>
#include <iostream>
#include <fstream>

#include "listener.h"
#include "singleton.hpp"

enum class FileMode : std::uint8_t
{
    DBG_OVERWRITE,
    DBG_APPEND,
    DBG_NEWFILE
};

enum class MsgType : std::uint8_t
{
    DBG_RAW,        // No effect gray message NO PARSING
    DBG_CANONICAL,  // No effect white message
    DBG_ITEM,       // Item in list
    DBG_TRACK,      // Relative to data/system being tracked NO PARSING
    DBG_NOTIFY,     // Relative to an event which should be notified to the user
    DBG_WARNING,    // Relative to an event which could impact the flow badly
    DBG_ERROR,      // Relative to a serious but recoverable error
                    // (eg. missing texture when you have a fall back one)
    DBG_FATAL,      // Relative to non recoverable error (eg. out of memory...)
    DBG_BANG,       // For code flow analysis
    DBG_GOOD,       // For test success
    DBG_BAD,        // For test fail
};

enum LogMode : std::uint8_t
{
    DBG_CONSOLE   = 1,
    DBG_SCREEN    = 2,
    DBG_FILE      = 4,
    DBG_CANONICAL = 5
};

// Holds a log message together with type and timestamp
struct LogMessage : public WData
{
public:
    typedef std::chrono::high_resolution_clock::time_point TimeStamp;

    LogMessage(const std::string& message, MsgType type=MsgType::DBG_CANONICAL,
                                           LogMode mode=LogMode::DBG_CANONICAL)
    : message_(message)
    , mode_(mode)
    , type_(type)
    , timestamp_(std::chrono::high_resolution_clock::now()) {}


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

    // Singleton boilerplate
    Logger (const Logger&)=delete;
    Logger();
   ~Logger();

    // Print to console with style given a message and its type
    void print_console(const std::string& message, MsgType type);
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
                     MsgType type=MsgType::DBG_CANONICAL,
                     LogMode mode=LogMode::DBG_CANONICAL);
    void operator ()(std::string&& message,
                     MsgType type=MsgType::DBG_CANONICAL,
                     LogMode mode=LogMode::DBG_CANONICAL);
    void operator ()(const LogMessage& log_message);

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

namespace dbg
{
    static Logger& LOG = Logger::Instance();  //Allows dbg::LOG("message",...) syntax
}

#ifdef __DEBUG__
    #define DLOG_SET(LOG_PATH, ...) do { \
        dbg::LOG.set(LOG_PATH, ## __VA_ARGS__); \
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
        dbg::LOG.track(HSTR_(MTYPESTR), INFORMER); \
        } while(0)

    #define DLOG(MESSAGE, ...) do { \
        dbg::LOG(MESSAGE, ## __VA_ARGS__); \
        } while(0)
    #define DLOGR(MESSAGE) do { \
        dbg::LOG(MESSAGE, MsgType::DBG_RAW); \
        } while(0)
    #define DLOGI(MESSAGE) do { \
        dbg::LOG(MESSAGE, MsgType::DBG_ITEM); \
        } while(0)
    #define DLOGT(MESSAGE) do { \
        dbg::LOG(MESSAGE, MsgType::DBG_TRACK); \
        } while(0)
    #define DLOGN(MESSAGE) do { \
        dbg::LOG(MESSAGE, MsgType::DBG_NOTIFY); \
        } while(0)
    #define DLOGW(MESSAGE) do { \
        dbg::LOG(MESSAGE, MsgType::DBG_WARNING); \
        } while(0)
    #define DLOGE(MESSAGE) do { \
        dbg::LOG(MESSAGE, MsgType::DBG_ERROR); \
        } while(0)
    #define DLOGF(MESSAGE) do { \
        dbg::LOG(MESSAGE, MsgType::DBG_FATAL); \
        } while(0)
    #define DLOGG(MESSAGE) do { \
        dbg::LOG(MESSAGE, MsgType::DBG_GOOD); \
        } while(0)
    #define DLOGB(MESSAGE) do { \
        dbg::LOG(MESSAGE, MsgType::DBG_BAD); \
        } while(0)
    #define BANG() do { \
        dbg::LOG(std::string(__FILE__)+":"+std::to_string(__LINE__), MsgType::DBG_BANG); \
        } while(0)
#else
    #define DLOG_SET(LOG_PATH, ...)     do { } while(0)
    #define DLOG_CLEAR()                do { } while(0)
    #define DLOG_WRITE()                do { } while(0)
    #define DLOG_PRINT_REF()            do { } while(0)
    #define DLOG_TRACK(MTYPE, INFORMER) do { } while(0)
    #define DLOG(MESSAGE, ...)          do { } while(0)
    #define DLOGR(MESSAGE)              do { } while(0)
    #define DLOGI(MESSAGE)              do { } while(0)
    #define DLOGT(MESSAGE)              do { } while(0)
    #define DLOGN(MESSAGE)              do { } while(0)
    #define DLOGW(MESSAGE)              do { } while(0)
    #define DLOGE(MESSAGE)              do { } while(0)
    #define DLOGF(MESSAGE)              do { } while(0)
    #define DLOGG(MESSAGE)              do { } while(0)
    #define DLOGB(MESSAGE)              do { } while(0)
    #define BANG(MESSAGE)               do { } while(0)
#endif

#endif // LOGGER_H
