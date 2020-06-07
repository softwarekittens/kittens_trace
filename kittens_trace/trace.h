#pragma once

#include <string_view>

#ifdef ENABLE_TRACING
#define TRACE_SCOPE kitten::ScopedTrace __traceMarker__##__LINE__(__FILE__, __func__, __LINE__)
#define TRACE_NAMED_SCOPE(name) kitten::ScopedTrace __traceMarker__##__LINE__(__FILE__, name, __LINE__)
#define TRACE_SET_THREAD_NAME(name) kitten::setTraceThreadName(name)
#else
#define TRACE_SCOPE
#define TRACE_NAMED_SCOPE(name)
#define TRACE_SET_THREAD_NAME(name) 
#endif

namespace kitten {

struct TraceMarker;

class ScopedTrace {
public:
    ScopedTrace(std::string_view file, std::string_view func, int line);
    ~ScopedTrace();
private:
    TraceMarker* m_marker;
};

void setTraceThreadName(std::string_view name);

void saveCollectedTracing(std::string_view filename);

}
