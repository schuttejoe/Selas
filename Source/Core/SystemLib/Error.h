#pragma once

//=================================================================================================================================
// Joe Schutte
//=================================================================================================================================

#include "SystemLib/BasicTypes.h"

namespace Selas
{
    class Error
    {
    private:
        char* errorMessage;
        uint32* refCountPtr;

        void IncrementRefCount();
        void DecrementRefCount();

    public:
        Error(const char* message, ...);
        Error()
            : errorMessage(nullptr)
            , refCountPtr(nullptr)
        {

        }
        ~Error();

        Error(const Error& error_);
        Error& operator=(const Error& error_);

        const char* Message() { return errorMessage; }
        bool Success() { return errorMessage == nullptr; }
        bool Failed() { return errorMessage != nullptr; }
    };


    #define Success_            Error()
    #define Successful_(error)  (error.Success())
    #define Failed_(error)      (error.Failed())

    #define Error_(messageStr, ...) Error(messageStr, ##__VA_ARGS__)
    #define ReturnError_(error_)                     \
    {                                                \
        Error returnErrorErr = error_;               \
        if(Failed_(returnErrorErr)) {                \
            return returnErrorErr;                   \
        }                                            \
    }
    #define ExitMainOnError_(error_)                 \
    {                                                \
        Error exitOnErrError = error_;               \
        if(Failed_(exitOnErrError)) {                \
            printf("%s", exitOnErrError.Message());  \
            return -1;                               \
        }                                            \
    }

};
