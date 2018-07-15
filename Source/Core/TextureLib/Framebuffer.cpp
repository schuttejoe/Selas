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
    void FrameBuffer_Initialize(Framebuffer* frame, uint32 width, uint32 height)
    {
        frame->width = width;
        frame->height = height;
        CreateSpinLock(frame->spinlock);
        frame->buffer = AllocArrayAligned_(float3, width * height, 16);

        Memory::Zero(frame->buffer, sizeof(float3) * width * height);
    }

    //=============================================================================================================================
    void FrameBuffer_Shutdown(Framebuffer* frame)
    {
        FreeAligned_(frame->buffer);
    }

    //=============================================================================================================================
    void FrameBuffer_Save(Framebuffer* frame, cpointer name)
    {
        FixedString128 root = Environment_Root();
        uint8 pathsep = StringUtil::PathSeperator();

        FilePathString dirpath;
        FixedStringSprintf(dirpath, "%s_Images%c", root.Ascii(), pathsep);

        FilePathString filepath;
        FixedStringSprintf(filepath, "%s%s.hdr", dirpath.Ascii(), name);

        Directory::EnsureDirectoryExists(dirpath.Ascii());

        StbImageWrite(filepath.Ascii(), frame->width, frame->height, 3, HDR, frame->buffer);
    }

    //=============================================================================================================================
    void FrameBuffer_Normalize(Framebuffer* __restrict frame, float term)
    {
        uint indexcount = frame->width * frame->height;
        for(uint scan = 0; scan < indexcount; ++scan) {
            frame->buffer[scan] = frame->buffer[scan] * term;
        }
    }

    //=============================================================================================================================
    void FramebufferWriter_Initialize(FramebufferWriter* writer, Framebuffer* frame, uint32 capacity, uint32 softCapacity)
    {
        writer->count = 0;
        writer->capacity = capacity;
        writer->softCapacity = softCapacity;
        writer->framebuffer = frame;

        writer->samples = AllocArrayAligned_(FramebufferSample, capacity, 16);
    }

    //=============================================================================================================================
    static void FlushInternal(FramebufferWriter* __restrict writer)
    {
        Assert_(*writer->framebuffer->spinlock == 1);

        Framebuffer* frame = writer->framebuffer;

        for(uint scan = 0, count = writer->count; scan < count; ++scan) {
            uint32 index = writer->samples[scan].index;
            frame->buffer[index] += writer->samples[scan].value;
        }

        writer->count = 0;
    }

    //=============================================================================================================================
    void FramebufferWriter_Write(FramebufferWriter* __restrict writer, float3 sample, uint32 x, uint32 y)
    {
        uint32 index = writer->framebuffer->width * y + x;
        FramebufferWriter_Write(writer, sample, index);
    }

    //=============================================================================================================================
    void FramebufferWriter_Write(FramebufferWriter* writer, float3 sample, uint32 index)
    {
        if(writer->count == writer->capacity) {
            FramebufferWriter_Flush(writer);
        }

        writer->samples[writer->count].index = index;
        writer->samples[writer->count].value = sample;
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
    }
}