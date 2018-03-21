#pragma once

//==============================================================================
// Joe Schutte
//==============================================================================

#include <SysteMlib/BasicTypes.h>
#include <SystemLib/Memory.h>

namespace Shooty
{
    //==============================================================================
    inline uint32 FloatToBits(float f)
    {
        uint32 u;
        Memory::Copy(&u, &f, sizeof(f));
        return u;
    }

    //==============================================================================
    inline float BitsToFloat(uint32 ui)
    {
        float f;
        Memory::Copy(&f, &ui, sizeof(uint32));
        return f;
    }

    //==============================================================================
    inline float NextFloatUp(float v)
    {
        // -- Handle negative zero
        if(v == -0.0f) {
            v = 0.0f;
        }

        uint32 ui = FloatToBits(v);

        // -- Handle infinity and negative infinity
        if(ui == FloatInfinityBits_ || ui == FloatNegativeInfinityBits_) {
            return v;
        }

        // -- Advance v to next higher float
        if(v >= 0) {
            ++ui;
        }
        else {
            --ui;
        }

        return BitsToFloat(ui);
    }

    //==============================================================================
    inline float NextFloatDown(float v)
    {
        if(v == -0.0f) {
            v = 0.0f;
        }

        uint32 ui = FloatToBits(v);

        // -- Handle infinity and negative infinity
        if(ui == FloatInfinityBits_ || ui == FloatNegativeInfinityBits_) {
            return v;
        }      
        
        if(v > 0) {
            --ui;
        }
        else {
            ++ui;
        }

        return BitsToFloat(ui);
    }

    //==============================================================================
    static float AdjustFloatOffset(float x)
    {
        if(x > 0) {
            return NextFloatUp(x);
        }
        else if(x < 0) {
            return NextFloatDown(x);
        }

        return x;
    }
}