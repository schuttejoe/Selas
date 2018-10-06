//=================================================================================================================================
// Joe Schutte
//=================================================================================================================================

#include "TextureLib/Framebuffer.h"
#include "TextureLib/StbImage.h"
#include "StringLib/FixedString.h"
#include "StringLib/StringUtil.h"
#include "MathLib/FloatFuncs.h"
#include "IoLib/Environment.h"
#include "IoLib/Directory.h"
#include "SystemLib/MemoryAllocation.h"
#include "SystemLib/Memory.h"

namespace Selas
{
    //=============================================================================================================================
    void FrameBuffer_Initialize(Framebuffer* frame, uint32 width, uint32 height, uint32 layerCount)
    {
        CreateSpinLock(frame->spinlock);

        frame->width = width;
        frame->height = height;
        frame->layerCount = layerCount;
        
        frame->buffers = AllocArray_(float3*, layerCount);
        for(uint scan = 0; scan < layerCount; ++scan) {
            frame->buffers[scan] = AllocArrayAligned_(float3, width * height, 16);
            Memory::Zero(frame->buffers[scan], sizeof(float3) * width * height);
        }
    }

    //=============================================================================================================================
    void FrameBuffer_Shutdown(Framebuffer* frame)
    {
        for(uint scan = 0; scan < frame->layerCount; ++scan) {
            FreeAligned_(frame->buffers[scan]);
        }
        Free_(frame->buffers);
    }

    //=============================================================================================================================
    void FrameBuffer_Save(Framebuffer* frame, cpointer name)
    {
        FixedString128 root = Environment_Root();
        uint8 pathsep = StringUtil::PathSeperator();

        FilePathString dirpath;
        FixedStringSprintf(dirpath, "%s_Images%c", root.Ascii(), pathsep);

        Directory::EnsureDirectoryExists(dirpath.Ascii());

        for(uint32 scan = 0, count = frame->layerCount; scan < count; ++scan) {
            FilePathString filepath;
            FixedStringSprintf(filepath, "%s%s_%u.hdr", dirpath.Ascii(), name, scan);

            StbImageWrite(filepath.Ascii(), frame->width, frame->height, 3, HDR, frame->buffers[scan]);
        }
    }

    //=============================================================================================================================
    void FrameBuffer_Scale(Framebuffer* __restrict frame, float term)
    {
        uint indexcount = frame->width * frame->height;
        for(uint layer = 0; layer < frame->layerCount; ++layer) {
            for(uint scan = 0; scan < indexcount; ++scan) {
                frame->buffers[layer][scan] = frame->buffers[layer][scan] * term;
            }
        }
    }

    //=============================================================================================================================
    void FramebufferWriter_Initialize(FramebufferWriter* writer, Framebuffer* frame, uint32 capacity, uint32 softCapacity)
    {
        writer->count = 0;
        writer->capacity = capacity;
        writer->softCapacity = softCapacity;
        writer->framebuffer = frame;

        writer->sampleIndices = AllocArrayAligned_(uint32, capacity, 16);
        writer->samples = AllocArrayAligned_(float3, frame->layerCount * capacity, 16);
    }

    //=============================================================================================================================
    static void FlushInternal(FramebufferWriter* __restrict writer)
    {
        Assert_(*writer->framebuffer->spinlock == 1);

        uint layerCount = writer->framebuffer->layerCount;

        Framebuffer* frame = writer->framebuffer;
        for(uint scan = 0, count = writer->count; scan < count; ++scan) {
            uint32 index = writer->sampleIndices[scan];

            for(uint layer = 0; layer < layerCount; ++layer) {
                frame->buffers[layer][index] += writer->samples[layerCount * scan + layer];
            }
        }
        
        writer->count = 0;
    }

    //=============================================================================================================================
    void FramebufferWriter_Write(FramebufferWriter* __restrict writer, float3* samples, uint32 layerCount, uint32 x, uint32 y)
    {
        uint32 index = writer->framebuffer->width * y + x;
        FramebufferWriter_Write(writer, samples, layerCount, index);
    }

    //=============================================================================================================================
    void FramebufferWriter_Write(FramebufferWriter* writer, float3* samples, uint32 layerCount, uint32 index)
    {
        if(writer->count == writer->capacity) {
            FramebufferWriter_Flush(writer);
        }

        Assert_(layerCount == writer->framebuffer->layerCount);

        writer->sampleIndices[writer->count] = index;
        for(uint layer = 0; layer < layerCount; ++layer) {
            writer->samples[layerCount * writer->count + layer] = samples[layer];
        }
        
        ++writer->count;

        if(writer->count > writer->softCapacity) {
            bool locked = TryEnterSpinLock(writer->framebuffer->spinlock);
            if(!locked) {
                return;
            }

            FlushInternal(writer);
            LeaveSpinLock(writer->framebuffer->spinlock);
        }
    }

    //=============================================================================================================================
    void FramebufferWriter_Flush(FramebufferWriter* writer)
    {
        EnterSpinLock(writer->framebuffer->spinlock);
        FlushInternal(writer);
        LeaveSpinLock(writer->framebuffer->spinlock);
    }

    //=============================================================================================================================
    void FramebufferWriter_Shutdown(FramebufferWriter* writer)
    {
        FramebufferWriter_Flush(writer);
        FreeAligned_(writer->samples);
        FreeAligned_(writer->sampleIndices);
    }
}