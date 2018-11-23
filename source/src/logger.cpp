#include <sstream>
#include <regex>
#include <iomanip>

#include "logger.h"
#include "informer.h"

#ifndef __DISABLE_EDITOR__
    #include "imgui/imgui.h"
#endif

namespace wcore
{

static std::map<MsgType, std::string> STYLES =
{
    {MsgType::RAW,       "\033[1;38;2;200;200;200m"},
    {MsgType::CANONICAL, "\033[1;38;2;255;255;255m"},
    {MsgType::ITEM,      "\033[1;38;2;255;255;255m"},
    {MsgType::TRACK,     "\033[1;38;2;75;255;75m"},
    {MsgType::NOTIFY,    "\033[1;38;2;75;75;255m"},
    {MsgType::SECTION,   "\033[1;38;2;10;10;10m"},
    {MsgType::WARNING,   "\033[1;38;2;255;175;0m"},
    {MsgType::ERROR,     "\033[1;38;2;255;90;90m"},
    {MsgType::FATAL,     "\033[1;38;2;255;0;0m"},
    {MsgType::BANG,      "\033[1;38;2;255;100;0m"},
    {MsgType::GOOD,      "\033[1;38;2;0;255;0m"},
    {MsgType::BAD,       "\033[1;38;2;255;0;0m"},
};

static std::map<char, std::string> TAG_STYLES =
{
    {'p', "\033[1;38;2;0;255;255m"},   // highlight paths in light blue
    {'n', "\033[1;38;2;255;50;0m"},    // names and symbols in dark orange
    {'i', "\033[1;38;2;255;190;10m"},  // instructions in light orange
    {'w', "\033[1;38;2;220;200;255m"},  // values in light purple
    {'v', "\033[1;38;2;190;10;255m"},  // important values in purple
    {'u', "\033[1;38;2;0;255;100m"},   // uniforms and attributes in light green
    {'d', "\033[1;38;2;255;100;0m"},   // default in vivid orange
    {'b', "\033[1;38;2;255;0;0m"},     // bad things in red
    {'g', "\033[1;38;2;0;255;0m"},     // good things in green
    {'z', "\033[1;38;2;255;255;255m"}, // neutral things in white
    {'x', "\033[1;38;2;0;206;209m"},   // XML nodes in turquoise
};

static std::map<MsgType, std::string> ICON =
{
    {MsgType::RAW,       "    "},
    {MsgType::CANONICAL, "    "},
    {MsgType::ITEM,      "     \u21B3 "},
    {MsgType::SECTION,   "\033[1;48;2;200;200;200m\u2042  \033[1;49m \033[1;48;2;200;200;200m"},
    {MsgType::TRACK,     "\033[1;48;2;50;50;50m \u03FE \033[1;49m "},
    {MsgType::NOTIFY,    "\033[1;48;2;20;10;50m \u2055 \033[1;49m "},
    {MsgType::WARNING,   "\033[1;48;2;50;40;10m \u203C \033[1;49m "},
    {MsgType::ERROR,     "\033[1;48;2;50;10;10m \u2020 \033[1;49m "},
    {MsgType::FATAL,     "\033[1;48;2;50;10;10m \u2021 \033[1;49m "},
    {MsgType::BANG,      "\033[1;48;2;50;40;10m \u0489 \033[1;49m "},
    {MsgType::GOOD,      "\033[1;48;2;10;50;10m \u203F \033[1;49m "},
    {MsgType::BAD,       "\033[1;48;2;50;10;10m \u2054 \033[1;49m "},
};

Logger::Logger()
: Listener()
, file_mode_(FileMode::OVERWRITE)
, messages_()
, start_time_(std::chrono::high_resolution_clock::now())
{
    //ctor
}

Logger::~Logger()
{

}

// Matches tagged text <X>text text text</X>. Tag must be single character.
// Puts tag name in $0 and tagged text in $1
static std::regex reg_tag("<(\\w)>(.+)</\\1>");

void Logger::parse_tags(const std::string& message, std::string& final, MsgType type)
{
    std::smatch m;
    final = message;
    while(std::regex_search(final, m, reg_tag))
    {
        if(m.size()==0) return;

        char c = m[1].str()[0];
        final = m.prefix().str() + TAG_STYLES[c] + m[2].str() + STYLES[type] + m.suffix().str();
    }
}

void Logger::strip_tags(const std::string& message, std::string& stripped, MsgType type)
{
    stripped = std::regex_replace(message, reg_tag, "$2");
}

void Logger::print_console(const std::string& message, MsgType type, float timestamp)
{
    // Set style
    std::cout << "\033[1;38;2;0;100;0m["
              << std::setprecision(10) << std::fixed
              << timestamp << "] ";
    std::cout << STYLES[type] << ICON[type];

    if(type == MsgType::RAW || type == MsgType::TRACK)
    {
        std::cout << message << "\033[0m" << std::endl;
        return;
    }

    std::string final;
    parse_tags(message, final, type);

    std::cout << final << "\033[0m" << std::endl;
    widget_scroll_required_ = true;
}

void Logger::operator ()(const std::string& message, MsgType type, LogMode mode)
{
    auto timestamp = std::chrono::high_resolution_clock::now() - start_time_;
    LogMessage logm(message, timestamp, type, mode);
    operator ()(logm);
}

void Logger::operator ()(std::string&& message, MsgType type, LogMode mode)
{
    auto timestamp = std::chrono::high_resolution_clock::now() - start_time_;
    LogMessage logm(message, timestamp, type, mode);
    operator ()(logm);
}

void Logger::operator ()(const LogMessage& log_message)
{
    if(((uint8_t)log_message.mode_ & (uint8_t)LogMode::TEXTFILE) != 0)
        messages_.push_back(log_message);

    if(((uint8_t)log_message.mode_ & (uint8_t)LogMode::CONSOLE) != 0)
    {
        float timestamp = std::chrono::duration_cast<std::chrono::duration<float>>(log_message.timestamp_).count();
        print_console(log_message.message_, log_message.type_, timestamp);
    }
}

#ifndef __DISABLE_EDITOR__
void Logger::generate_widget()
{
    ImGuiTextFilter Filter;
    ImGui::SetNextWindowSize(ImVec2(500,400), ImGuiSetCond_FirstUseEver);
    ImGui::Begin("Log");
    if(ImGui::Button("Clear")) clear();
    ImGui::SameLine();
    bool copy = ImGui::Button("Copy");
    ImGui::SameLine();
    Filter.Draw("Filter", -100.0f);
    ImGui::Separator();
    ImGui::BeginChild("scrolling");
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0,1));
    if(copy) ImGui::LogToClipboard();

    if(Filter.IsActive())
    {
        for(uint32_t ii=0; ii<messages_.size(); ++ii)
        {
            const char* line_begin = messages_[ii].message_.c_str();
            if(Filter.PassFilter(line_begin))
                ImGui::TextUnformatted(line_begin);
        }
    }
    else
    {
        for(uint32_t ii=0; ii<messages_.size(); ++ii)
        {
            ImGui::TextUnformatted(messages_[ii].message_.c_str());
        }
    }

    if (widget_scroll_required_)
        ImGui::SetScrollHere(1.0f);
    widget_scroll_required_ = false;
    ImGui::PopStyleVar();
    ImGui::EndChild();
    ImGui::End();
}
#endif

void Logger::onTrack(const WData& data)
{
    auto timestamp = std::chrono::high_resolution_clock::now() - start_time_;
    LogMessage LM("Informer: " + std::to_string(data.sender_) + " >> " + data.to_string(),
                  timestamp,
                  MsgType::TRACK);
    operator()(LM);
}

void Logger::track(hash_t chan, Informer& informer)
{
    subscribe(chan, informer, &Logger::onTrack);
    std::stringstream ss;
    ss << "[LOG] Listening to channel " << chan << " from informer "
       << informer.get_WID();
    operator()(ss.str(), MsgType::NOTIFY);
}

void Logger::untrack(hash_t chan, Informer& informer)
{
    unsubscribe(chan, informer);
    std::stringstream ss;
    ss << "[LOG] Ignoring channel " << chan << " from informer "
       << informer.get_WID();
    operator()(ss.str(), MsgType::NOTIFY);
}

void Logger::untrack(Informer& informer)
{
    for(auto key: delegate_ids_)
        if(key.first.second == informer.get_WID())
            unsubscribe(key.first.first, informer);

    operator()("[LOG] Ignoring messages from informer " + std::to_string(informer.get_WID()),
               MsgType::NOTIFY);
}

void Logger::write(const std::string& log_path)
{
    std::ofstream log(log_path.c_str());
    log << "-------------------------" << std::endl;
    log << "[TIME STAMP] [MTYPE] [MESSAGE]" << std::endl;
    for(const LogMessage& logmsg : messages_)
    {
        float timestamp = std::chrono::duration_cast<std::chrono::duration<float>>(logmsg.timestamp_).count();

        log << "[" << timestamp << "] ";
        log << ICON[logmsg.type_];
        log << logmsg.message_ << std::endl;
    }
    log.close();
}

void Logger::print_reference()
{
    print_console("Section.", MsgType::SECTION);
    print_console("Raw message.", MsgType::RAW);
    print_console("Canonical message.", MsgType::CANONICAL);
    print_console("Item message.", MsgType::ITEM);
    print_console("Notify message.", MsgType::NOTIFY);
    print_console("Tracking message.", MsgType::TRACK);
    print_console("Warning message.", MsgType::WARNING);
    print_console("Error message.", MsgType::ERROR);
    print_console("Fatal Error message.", MsgType::FATAL);
    print_console("Test success.", MsgType::GOOD);
    print_console("Test fail.", MsgType::BAD);
    std::cout << std::endl;
}

}
