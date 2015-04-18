#pragma once

#include "trace_file.hpp"
#include "trace_parser.hpp"

#include <string>
#include <map>
#include <queue>
#include <vector>

namespace trace  {

class Frame;

struct RenderBookmark {
    RenderBookmark()
        : numberOfCalls(0)
    {}
    RenderBookmark(const ParseBookmark &s)
        : start(s),
          numberOfCalls(0)
    {}
    ParseBookmark start;
    unsigned numberOfCalls;
};    

struct FrameBookmark {
    FrameBookmark()
        : numberOfCalls(0)
    {}
    FrameBookmark(const ParseBookmark &s)
        : start(s),
          numberOfCalls(0)
    {}
    unsigned numberOfRenders() const;
    std::vector<RenderBookmark> renders() const;

    ParseBookmark start;
    unsigned numberOfCalls;
};

class Loader
{
public:
    enum FrameMarker {
        FrameMarker_SwapBuffers,
        FrameMarker_Flush,
        FrameMarker_Finish,
        FrameMarker_Clear
    };
public:
    Loader();
    ~Loader();

    Loader::FrameMarker frameMarker() const;
    void setFrameMarker(Loader::FrameMarker marker);

    unsigned numberOfFrames() const;
    unsigned numberOfCallsInFrame(unsigned frameIdx) const;
    std::vector<FrameBookmark> frames() const;

    bool open(const char *filename);
    void close();

    std::vector<trace::Call*> frame(unsigned idx);

private:
    bool isCallAFrameMarker(const trace::Call *call) const;

private:
    trace::Parser m_parser;
    FrameMarker m_frameMarker;

    typedef std::map<int, FrameBookmark> FrameBookmarks;
    FrameBookmarks m_frameBookmarks;
};

}

