#pragma once

//=================================================================================================================================
// Joe Schutte
//=================================================================================================================================

#include "MathLib/FloatStructs.h"
#include "SystemLib/Error.h"
#include "SystemLib/BasicTypes.h"
#include "SystemLib/OSThreading.h"

namespace Selas
{
    #define DefaultFrameWriterCapacity_     4096
    #define DefaultFrameWriterSoftCapacity_ 3840

    struct Framebuffer
    {
        uint32  width;
        uint32  height;
        uint32  layerCount;
        float3** buffers;
        uint8 spinlock[CacheLineSize_];
    };

    struct FramebufferWriter
    {
        uint32  count;
        uint32  capacity;
        uint32  softCapacity;
        uint32  pad;
        uint32* sampleIndices;
        float3* samples;
        Framebuffer* framebuffer;
    };

    void FrameBuffer_Initialize(Framebuffer* frame, uint32 width, uint32 height, uint32 layerCount);
    void FrameBuffer_Shutdown(Framebuffer* frame);
    void FrameBuffer_Save(Framebuffer* frame, cpointer name);
    void FrameBuffer_Normalize(Framebuffer* frame, float value);

    void FramebufferWriter_Initialize(FramebufferWriter* writer, Framebuffer* frame, uint32 capacity, uint32 softCapacity);
    void FramebufferWriter_Write(FramebufferWriter* writer, float3* samples, uint32 layerCount, uint32 x, uint32 y);
    void FramebufferWriter_Write(FramebufferWriter* writer, float3* samples, uint32 layerCount, uint32 index);
    void FramebufferWriter_Flush(FramebufferWriter* writer);
    void FramebufferWriter_Shutdown(FramebufferWriter* writer);
}