

#include "VivoxTracer.h"
#include "FightingVR.h"

DEFINE_LOG_CATEGORY(LogFightingVRTracer);

void Tracer::_LogVerbose(const FString Name, const FString Message)
{
    SET_WARN_COLOR( COLOR_CYAN );
    UE_LOG(LogFightingVRTracer, Verbose, TEXT("%s() : %s"), *Name, *Message);
    CLEAR_WARN_COLOR();
}

void Tracer::_LogVeryVerbose(const FString Name, const FString Message)
{
    SET_WARN_COLOR( COLOR_BLUE );
    UE_LOG(LogFightingVRTracer, VeryVerbose, TEXT("%s() : %s"), *Name, *Message);
    CLEAR_WARN_COLOR();
}

void Tracer::_MethodPrologue(const FString Name, const FString Message)
{
    _LogVerbose(Name, TEXT("Entered."));
    if (!Message.IsEmpty())
    {
        _LogVeryVerbose(Name, Message);
    }
}
