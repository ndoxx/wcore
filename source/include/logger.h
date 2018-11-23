#ifndef LOGGER_H
#define LOGGER_H

#include <string>
#include <vector>
#include <map>
#include <chrono>
#include <ctime>
#include <iostream>
#include <fstream>
#include <sstream>
#include <algorithm>

#include "listener.h"
#include "singleton.hpp"
#include "utils.h"

namespace wcore
{

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

enum Severity : std::uint32_t
{
    DET  = 0, // Detail
    LOW  = 1, // Informative stuff
    WARN = 2, // Warning (can recover)
    CRIT = 3  // Critical (error and fatal error)
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
               LogMode mode=LogMode::CANONICAL,
               uint32_t severity=0,
               hashstr_t channel=HS_("default"))
    : message_(message)
    , mode_(mode)
    , type_(type)
    , channel_(channel)
    , timestamp_(timestamp)
    {
        init(severity);
    }

    LogMessage(std::string&& message,
               const TimeStamp& timestamp,
               MsgType type=MsgType::CANONICAL,
               LogMode mode=LogMode::CANONICAL,
               uint32_t severity=0,
               hashstr_t channel=HS_("default"))
    : message_(std::move(message))
    , mode_(mode)
    , type_(type)
    , channel_(channel)
    , timestamp_(timestamp)
    {
        init(severity);
    }

    std::string message_;
    LogMode     mode_;
    MsgType     type_;
    hashstr_t   channel_;
    uint32_t    verbosity_level;

    const TimeStamp timestamp_;

private:
    inline void init(uint32_t severity)
    {
        verbosity_level = 3u - std::min(severity, 3u);
    }
};

class Logger : public Singleton<Logger>, public Listener
{
private:
    FileMode file_mode_;               // What to do with the log file (new, overwrite, append)
    bool widget_scroll_required_;      // When new message logged, widget needs to scroll down
    LogMessage::TimePoint start_time_; // Start time for timestamp handling

    std::vector<LogMessage> messages_;          // List of logged messages
    std::map<hashstr_t, uint32_t> verbosity_;   // Map of channels verbosity levels
    std::map<hashstr_t, std::string> channels_; // Map of channel names

    // Singleton boilerplate
    Logger (const Logger&)=delete;
    Logger();
   ~Logger();

    // Print to console with style given a message and its type
    void print_console(const std::string& message, MsgType type, float timestamp=0.f);
    void print_console(const LogMessage& log_message);
    // Replace xml style tags within messages by ANSI escape sequences for styling
    void parse_tags(const std::string& message, std::string& final, MsgType type);
    // Strip xml tags from message
    void strip_tags(const std::string& message, std::string& stripped, MsgType type);

public:
    // Singleton boilerplate
    friend Logger& Singleton<Logger>::Instance();
    friend void Singleton<Logger>::Kill();

    // Register a debugging channel
    void register_channel(const char* name, uint32_t verbosity=0);
    // Change channel verbosity
    inline void set_channel_verbosity(hashstr_t name, uint32_t verbosity)
    {
        verbosity_.at(name) = std::min(verbosity, 3u);
    }
    // Mute channel by setting its verbosity to 0
    inline void mute_channel(hashstr_t name)
    {
        verbosity_.at(name) = 0;
    }
    // Get channel verbosity by name
    inline uint32_t get_channel_verbosity(hashstr_t name) const
    {
        return verbosity_.at(name);
    }
    // Get channel verbosity reference
    inline uint32_t& get_channel_verbosity_nc(hashstr_t name)
    {
        return verbosity_.at(name);
    }

    // Actual functions used for logging (functor style)
    void operator ()(const std::string& message,
                     MsgType type=MsgType::CANONICAL,
                     LogMode mode=LogMode::CANONICAL,
                     uint32_t severity=0u,
                     hashstr_t channel=HS_("default"));
    void operator ()(std::string&& message,
                     MsgType type=MsgType::CANONICAL,
                     LogMode mode=LogMode::CANONICAL,
                     uint32_t severity=0u,
                     hashstr_t channel=HS_("default"));
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
} // namespace dbg
} // namespace wcore

// DLOGx macros in GLOBAL namespace

#ifdef __DEBUG__
    #define DLOG_SET(LOG_PATH, ...) do { \
        wcore::dbg::LOG.set(LOG_PATH, ##__VA_ARGS__); \
        } while(0)
    #define DLOG_CLEAR() do { \
        wcore::dbg::LOG.clear(); \
        } while(0)
    #define DLOG_WRITE() do { \
        wcore::dbg::LOG.write(); \
        } while(0)
    #define DLOG_PRINT_REF() do { \
        wcore::dbg::LOG.print_reference(); \
        } while(0)
    #define DLOG_TRACK(MTYPESTR, INFORMER) do { \
        wcore::dbg::LOG.track(H_(MTYPESTR), INFORMER); \
        } while(0)

    #define FDLOG( ... ) do { \
        wcore::dbg::LOG.printfold(__VA_ARGS__); \
        } while(0)
    /*#define DLOG(MESSAGE, ...) do { \
        wcore::dbg::LOG( MESSAGE, ##__VA_ARGS__ ); \
        } while(0)*/
    #define DLOG(MESSAGE, CHANNEL, SEVERITY) do { \
        wcore::dbg::LOG( MESSAGE, MsgType::CANONICAL, LogMode::CANONICAL, SEVERITY, HS_( CHANNEL ) ); \
        } while(0)
    #define DLOGR(MESSAGE, CHANNEL, SEVERITY) do { \
        wcore::dbg::LOG( MESSAGE, MsgType::RAW,     LogMode::CANONICAL, SEVERITY, HS_( CHANNEL ) ); \
        } while(0)
    #define DLOGI(MESSAGE, CHANNEL, SEVERITY) do { \
        wcore::dbg::LOG( MESSAGE, MsgType::ITEM,    LogMode::CANONICAL, SEVERITY, HS_( CHANNEL ) ); \
        } while(0)
    #define DLOGT(MESSAGE, CHANNEL, SEVERITY) do { \
        wcore::dbg::LOG( MESSAGE, MsgType::TRACK,   LogMode::CANONICAL, SEVERITY, HS_( CHANNEL ) ); \
        } while(0)
    #define DLOGN(MESSAGE, CHANNEL, SEVERITY) do { \
        wcore::dbg::LOG( MESSAGE, MsgType::NOTIFY,  LogMode::CANONICAL, SEVERITY, HS_( CHANNEL ) ); \
        } while(0)
    #define DLOGS(MESSAGE, CHANNEL, SEVERITY) do { \
        wcore::dbg::LOG( MESSAGE, MsgType::SECTION, LogMode::CANONICAL, SEVERITY, HS_( CHANNEL ) ); \
        } while(0)
    #define DLOGW(MESSAGE, CHANNEL, SEVERITY) do { \
        wcore::dbg::LOG( MESSAGE, MsgType::WARNING, LogMode::CANONICAL, SEVERITY, HS_( CHANNEL ) ); \
        } while(0)
    #define DLOGE(MESSAGE, CHANNEL, SEVERITY) do { \
        wcore::dbg::LOG( MESSAGE, MsgType::ERROR,   LogMode::CANONICAL, SEVERITY, HS_( CHANNEL ) ); \
        } while(0)
    #define DLOGF(MESSAGE, CHANNEL, SEVERITY) do { \
        wcore::dbg::LOG( MESSAGE, MsgType::FATAL,   LogMode::CANONICAL, SEVERITY, HS_( CHANNEL ) ); \
        } while(0)
    #define DLOGG(MESSAGE, CHANNEL, SEVERITY) do { \
        wcore::dbg::LOG( MESSAGE, MsgType::GOOD,    LogMode::CANONICAL, SEVERITY, HS_( CHANNEL ) ); \
        } while(0)
    #define DLOGB(MESSAGE, CHANNEL, SEVERITY) do { \
        wcore::dbg::LOG( MESSAGE, MsgType::BAD,     LogMode::CANONICAL, SEVERITY, HS_( CHANNEL ) ); \
        } while(0)
    #define BANG() do { \
        wcore::dbg::LOG(std::string(__FILE__)+":"+std::to_string(__LINE__), MsgType::BANG); \
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
