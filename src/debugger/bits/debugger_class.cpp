#include "debugger_class.h"

#ifndef KMC_NO_DEBUG

#include "../../site.h"
#include "../../reactions/reaction.h"

#include <fstream>
#include <sys/time.h>

#include "intrinsicmacros.h"


using namespace kMC;


bool Debugger::enabled = true;
bool Debugger::prevState = true;

std::vector<std::string> Debugger::reactionTraceBefore;
std::vector<std::string> Debugger::reactionTraceAfter;
std::vector<std::string> Debugger::implicationTrace;
std::vector<double>      Debugger::timerData;

std::set<Site*>          Debugger::affectedUnion;


std::string Debugger::implications;
std::string Debugger::reactionString;
std::string Debugger::_pre = "";


Reaction* Debugger::currentReaction;
Reaction* Debugger::lastCurrentReaction;

uint Debugger::traceCount;
uint Debugger::implicationCount;

std::string Debugger::traceFileName = "";
std::string Debugger::traceFilePath = "";

wall_clock Debugger::timer;


void Debugger::setFilename(const string & filename)
{
    Debugger::traceFileName = filename;
}

void Debugger::setFilepath(const string & filepath)
{
    Debugger::traceFilePath = filepath;
}

void Debugger::setEnabledTo(bool state)
{

    prevState = enabled;

    enabled = state;

}

void Debugger::resetEnabled()
{
    enabled = prevState;
}

void Debugger::pushTraces()
{
    if (!enabled) return;

    timerData.push_back(timer.toc());

    stringstream s;

    if (affectedUnion.size() != Site::affectedSites().size())
    {
        s << setupAffectedUnion();

        s << addFlagsToImplications();

        implications += s.str();
    }

    implicationTrace.push_back(implications);

    reactionTraceBefore.push_back(reactionString);

    if (currentReaction != NULL)
    {
        reactionTraceAfter.push_back(_KMCDebugger_SITE_STR(currentReaction->getReactionSite()));
    }

    else
    {
        reactionTraceAfter.push_back("");
    }

    currentReaction = NULL;
    implications = _KMCDebugger_INITIAL_IMPLICATION_MSG;
    implicationCount = 0;
    reactionString = "No Reaction Selected";
    traceCount++;
    affectedUnion.clear();
    timer.tic();

}

void Debugger::pushImplication(Site * site, const char * _new)
{

    if (!enabled) return;

    using namespace std;

    stringstream s;

    s << "[Implication"; \
    if (currentReaction == NULL) s << " (standalone)";
    s << " " << implicationCount << "]\n";
    s << _KMCDebugger_MAKE_IMPLICATION_MESSAGE(site, _pre, _new);
    _pre = "";

    s << setupAffectedUnion();

    s << "[End of implication";
    if (currentReaction == NULL) s << " (standalone)";
    s << " " << implicationCount << "]\n\n";
    implications += s.str();

    implicationCount++;

}

void Debugger::markPartialStep(const char * msg)
{

    if (!enabled) return;

    using namespace std;

    stringstream s;

    s << "##### " << msg << "  " << "prev. imp.: " << implicationCount << " #####\n\n";

    s << addFlagsToImplications();

    string full_string = s.str();

    implications += full_string;

    implicationCount = 0;

}

void Debugger::setActiveReaction(Reaction *reaction)
{

    if (!enabled) return;

    currentReaction = reaction;
    lastCurrentReaction = reaction;
    reactionString = _KMCDebugger_REACTION_STR();
}

void Debugger::initialize()
{
    if (!enabled) return;

    reset();

    currentReaction = NULL;
    lastCurrentReaction = NULL;

    implications = _KMCDebugger_INITIAL_IMPLICATION_MSG;
    reactionString = _KMCDebugger_INITIAL_REACTION_STR;
    _pre = "";

    traceCount = 0;
    implicationCount = 0;

    (void)timer.toc();

    timer.tic();
}

std::string Debugger::setupAffectedUnion()
{
    using namespace std;

    stringstream s;

    vector<Site*> intersect;

    bool currentSiteInPrev;

    for (Site * currentSite : Site::affectedSites())
    {

        currentSiteInPrev = false;
        for (Site * previousSite : affectedUnion)
        {
            if (currentSite == previousSite)
            {
                currentSiteInPrev = true;
                break;
            }
        }

        if (!currentSiteInPrev)
        {
            intersect.push_back(currentSite);
        }
    }

    int X, Y, Z;
    for (Site * site : intersect)
    {
        s << "   -" << site->str();

        if (currentReaction != NULL)
        {
            currentReaction->getReactionSite()->distanceTo(site, X, Y, Z);
            s << " [" << X << ", " << Y  << ", " << Z << "]";
        }

        s << "\n";

        affectedUnion.insert(site);
    }

    if (intersect.size() == 0)
    {
        return "";
    }

    s << "Total: " << intersect.size() << endl;
    intersect.clear();

    KMCDebugger_Assert(affectedUnion.size(), ==, Site::affectedSites().size());

    return "New affected site(s):\n" + s.str();

}

std::string Debugger::addFlagsToImplications()
{

#ifdef KMC_VERBOSE_DEBUG
    stringstream s, sitess, reacss;

    for (Site * site : affectedUnion)
    {
        bool addSite = false;

        sitess << "\n" << site->info() << "\n";

        site->forEachActiveReactionDo([&] (Reaction * r)
        {

            reacss << "    .    " << r->str() << " Flag: " << r->updateFlag();

            addSite = true;

            sitess << reacss.str() << "\n";

            reacss.str(string());

        });


        if (addSite)
        {
            s << sitess.str();
        }

        sitess.str(string());
    }

    if (s.str().empty())
    {
        return "";
    }


    return "New non-default flags given to affected site reactions:\n\n" + s.str();

#else
    return "";
#endif

}

void Debugger::dumpFullTrace(int line, const char * filename, const string additionalInfo)
{
    if (!enabled) return;


    using namespace std;

    ofstream file;
    stringstream path;


    if (currentReaction != NULL || !(implications.compare(_KMCDebugger_INITIAL_IMPLICATION_MSG)==0))
    {
        pushTraces();
    }

    if (!traceFilePath.empty())
    {
        path << traceFilePath << "/";
    }

    path << "trace_";

    if (!traceFileName.empty())
    {
        path << traceFileName;
    }

    else
    {
        path << time(NULL);
    }

    path << ".txt";

    file.open(path.str().c_str());
    file << fullTrace(line, filename, additionalInfo);
    file.close();

    cout << "Trace saved to " << path.str() << endl;

}

void Debugger::dumpPartialTrace(const int &i)
{

    if (!enabled) return;

    using namespace std;

    assert(i >= 0);

    cout << partialTrace(i);
}


string Debugger::fullTrace(int line, const string filename, const string additionalInfo)
{
    using namespace std;

    stringstream s;


    s << "=====================\n TRACING REACTIONS \n=====================\n" << endl;


    for (uint i = 0; i < traceCount; ++i)
    {
        s << partialTrace(i);
    }


    s << "Full trace initiated at line " << line;
    s << "\nin " << filename << "\n";
    s << "=====================\n  TRACE FINISHED  \n=====================\n" << endl;

    if (!additionalInfo.empty())
    {
        s << "\nFinalizing debug info:\n============================\n\n";

        s << additionalInfo;
    }


    s << endl;

    return s.str();

}

string Debugger::partialTrace(const uint &i)
{
    using namespace std;

    stringstream s;

    s << "---[" << i << " / " << traceCount-1 << " ] " << timerData.at(i)*1000 << " ms" << endl;

    s << reactionTraceBefore.at(i) << endl;

    s << implicationTrace.at(i);

#ifdef KMC_VERBOSE_DEBUG
    if (!reactionTraceAfter.at(i).empty())
    {
        s << "\nEnd of reaction look from initial reaction site view: " << endl;
        s << reactionTraceAfter.at(i);
    }
#endif

    s << endl;

    return s.str();

}

void Debugger::reset()
{

    reactionTraceBefore.clear();
    reactionTraceAfter.clear();
    implicationTrace.clear();
    timerData.clear();

    affectedUnion.clear();

}

#endif
