//==============================================================================
// Joe Schutte
//==============================================================================

#include "TextureLib/TextureFiltering.h"
#include "MathLib/Trigonometric.h"

namespace Selas
{
    //==============================================================================
    namespace TextureFiltering
    {
        //==============================================================================
        void InitializeEWAFilterWeights()
        {
            for(uint i = 0; i < EwaLutSize; ++i) {
                float alpha = 2.0f;
                float r2 = float(i) / (EwaLutSize - 1.0f);
                EWAFilterLut[i] = Math::Expf(-alpha * r2) - Math::Expf(-alpha);
            }
        }
    }
}