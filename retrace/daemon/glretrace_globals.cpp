#include "trace_parser.hpp"
#include "trace_profiler.hpp"
#include "retrace.hpp"
#include "trace_callset.hpp"

static trace::CallSet snapshotFrequency;
static trace::ParseBookmark lastFrameStart;

static unsigned dumpStateCallNo = ~0;

retrace::Retracer retracer;

namespace retrace {

trace::Parser _parser;
trace::AbstractParser *parser = &_parser;
trace::Profiler profiler;


int verbosity = 0;
unsigned debug = 1;
bool dumpingState = false;
bool dumpingSnapshots = false;

Driver driver = DRIVER_DEFAULT;
const char *driverModule = NULL;

bool doubleBuffer = true;
unsigned samples = 1;

bool profiling = false;
bool profilingGpuTimes = false;
bool profilingCpuTimes = false;
bool profilingPixelsDrawn = false;
bool profilingMemoryUsage = false;
bool useCallNos = true;
bool singleThread = false;
bool markers = false;
bool profilingWithBackends = false;
bool profilingListMetrics = false;
bool curPas = false;
bool profilingNumPasses = false;
char *profilingCallsMetricsString = NULL;
char *profilingFramesMetricsString = NULL;
char *profilingDrawCallsMetricsString = NULL;
unsigned frameNo = 0;
unsigned callNo = 0;
unsigned int numPasses = 0;
unsigned int curPass = 0;

void
frameComplete(trace::Call &call) {
}

class DefaultDumper: public Dumper
{
public:
    int
    getSnapshotCount(void) override {
        return 0;
    }

    image::Image *
    getSnapshot(int n) override {
        return NULL;
    }

    bool
    canDump(void) override {
        return false;
    }

    void
    dumpState(StateWriter &writer) override {
        assert(0);
    }
};

static DefaultDumper defaultDumper;

Dumper *dumper = &defaultDumper;

}

