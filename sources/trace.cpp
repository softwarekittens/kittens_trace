#include "kitten_logger/trace.h"

#include <deque>
#include <chrono>
#include <mutex>
#include <vector>
#include <thread>
#include <string>
#include <sstream>
#include <fstream>
#include <set>
#include <memory>

namespace kitten {

// Метка трассировки
struct TraceMarker {
    std::string_view m_file;
    std::string_view m_func;
    int64_t m_begin;
    int64_t m_end;
    int m_line;

    void write(std::ofstream& stream, uint64_t threadId) const;
};

// Все собранные метки в thread-е
struct TraceThreadMarkers {
    std::deque<TraceMarker> m_markers;
    std::string m_threadName;
    uint64_t m_threadId;

    void write(std::ofstream& stream) const;
};

// per-thread объект для сбора меток
class TraceThreadManager {
public:
    TraceThreadManager();
    TraceMarker* createMarker();
    void setThreadName(std::string_view name);

private:
    std::shared_ptr<TraceThreadMarkers> m_markers;
    // Хак: указатель для быстрого доступа к контейнеру, перф тут слишком важен, чтобы работать через shared_ptr
    TraceThreadMarkers* m_markersPtr;
};

// Глобальный синглтон
class TraceManager {
public:
    void writeAll(std::ofstream& stream);
    void clearAll();
    void addThreadMarkers(const std::shared_ptr<TraceThreadMarkers>& threadMarkers);

private:
    std::mutex m_mutex;
    std::vector<std::shared_ptr<TraceThreadMarkers>> m_traceThreads;
};

}

namespace {
static kitten::TraceManager s_traceManager;
static thread_local kitten::TraceThreadManager s_traceThreadManager;
}

namespace kitten {

void TraceMarker::write(std::ofstream& stream, size_t threadId) const {
    stream << "{";
    stream << "\"pid\":1,";
    stream << "\"tid\":" << threadId << ",";
    stream << "\"ts\":" << m_begin << ",";
    stream << "\"ph\":\"B\",";
    stream << "\"cat\":" << "\"high\"" << ",";
    stream << "\"name\":\"" << m_func << "\",";
    stream << "\"args\":{}";
    stream << "},\n";

    stream << "{";
    stream << "\"pid\":1,";
    stream << "\"tid\":" << threadId << ",";
    stream << "\"ts\":" << m_end << ",";
    stream << "\"ph\":\"E\",";
    stream << "\"cat\":" << "\"high\"" << ",";
    stream << "\"name\":\"" << m_func << "\",";
    stream << "\"args\":{}";
    stream << "}";
}

void TraceThreadMarkers::write(std::ofstream& stream) const {
    bool isFirst = true;
    for (const auto& marker: m_markers) {
        if (isFirst) {
            isFirst = false;
        } else {
            stream << ",\n";
        }
        marker.write(stream, m_threadId);
    }
}

TraceThreadManager::TraceThreadManager() {
    m_markers = std::make_shared<TraceThreadMarkers>();

    // Хак по конвертации std::thread::id в uint64_t. TODO: исправить
    std::stringstream ss;
    ss << std::this_thread::get_id();
    m_markers->m_threadId = std::stoull(ss.str());

    m_markersPtr = m_markers.get();
    s_traceManager.addThreadMarkers(m_markers);
}

TraceMarker* TraceThreadManager::createMarker() {
    m_markersPtr->m_markers.push_back(TraceMarker());
    return &m_markersPtr->m_markers.back();
}

void TraceThreadManager::setThreadName(std::string_view name) {
    m_markersPtr->m_threadName = name;
}

void TraceManager::writeAll(std::ofstream& stream) {
    std::unique_lock lock(m_mutex);
    bool isFirst = true;
    for (auto traceThread: m_traceThreads) {
        if (traceThread->m_markers.empty()) {
            continue;
        }

        if (isFirst) {
            isFirst = false;
        } else {
            stream << ",\n";
        }
        traceThread->write(stream);
    }
}

void TraceManager::clearAll() {
    std::unique_lock lock(m_mutex);
    for (auto traceThread: m_traceThreads) {
        traceThread->m_markers.clear();
    }
}

void TraceManager::addThreadMarkers(const std::shared_ptr<TraceThreadMarkers>& threadMarkers) {
    std::unique_lock lock(m_mutex);
    m_traceThreads.push_back(threadMarkers);
}

ScopedTrace::ScopedTrace(std::string_view file, std::string_view func, int line) {
    m_marker = s_traceThreadManager.createMarker();
    m_marker->m_file = file;
    m_marker->m_func = func;
    m_marker->m_line = line;
    m_marker->m_begin = std::chrono::duration_cast<std::chrono::microseconds>(
        std::chrono::high_resolution_clock::now().time_since_epoch()).count();
    m_marker->m_end = m_marker->m_begin;
}

ScopedTrace::~ScopedTrace() {
    m_marker->m_end = std::chrono::duration_cast<std::chrono::microseconds>(
        std::chrono::high_resolution_clock::now().time_since_epoch()).count();
}

void setTraceThreadName(std::string_view name) {
    s_traceThreadManager.setThreadName(name);
}

void saveCollectedTracing(std::string_view filename) {
    std::ofstream stream;
    stream.open(std::string(filename));
    stream << "[\n";
    s_traceManager.writeAll(stream);
    s_traceManager.clearAll();
    stream << "\n]\n";
    stream.close();
}

}
