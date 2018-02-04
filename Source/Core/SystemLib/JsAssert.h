#pragma once

//==============================================================================
// Joe Schutte
//==============================================================================

#define AllowAsserts_ 1

namespace Shooty {

  void AssertHandler(const char* message, const char* filename, int line);

};

#define Assert_(exp)             if(!(exp)) Shooty::AssertHandler("Unspecified Assert", __FILE__, __LINE__);
#define AssertMsg_(exp, message) if(!(exp)) Shooty::AssertHandler(message, __FILE__, __LINE__);
#define Error_(message)          Shooty::AssertHandler(message, __FILE__, __LINE__);