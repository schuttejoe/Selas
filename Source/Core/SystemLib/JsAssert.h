#pragma once

//==============================================================================
// Joe Schutte
//==============================================================================

#define AllowAsserts_ 1

namespace Selas {

  void AssertHandler(const char* message, const char* filename, int line);

};

#define Assert_(exp)             if(!(exp)) Selas::AssertHandler("Unspecified Assert", __FILE__, __LINE__);
#define AssertMsg_(exp, message) if(!(exp)) Selas::AssertHandler(message, __FILE__, __LINE__);
#define Error_(message)          Selas::AssertHandler(message, __FILE__, __LINE__);
