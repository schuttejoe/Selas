#pragma once

//==============================================================================
// Joe Schutte
//==============================================================================

#include <SystemLib\BasicTypes.h>
#include <ContainersLib\CArray.h>

namespace Shooty {

  struct PointerDesc {
    uint64 pointerOffset;
    uint64 pointerDataOffset;
  };

  //==============================================================================
  struct BinaryWriter
  {
    void*     file;

    CArray<uint8>       rawData;
    CArray<PointerDesc> pointers;
    CArray<uint8>       pointerData;
  };

  //==============================================================================
  struct BinaryReader
  {
    void*   data;
    uint32  size;
    uint32  offset;
  };

  // Write interface
  bool SerializerStart (BinaryWriter* serializer, const char* filename);
  bool SerializerEnd   (BinaryWriter* serializer);
  bool SerializerWrite (BinaryWriter* serializer, const void* data, uint size);

  bool SerializerWritePointerData (BinaryWriter* serializer, const void* data, uint size);
  bool SerializerWritePointerOffsetX64 (BinaryWriter* serializer);

  // Read interface
  void SerializerStart   (BinaryReader* serializer, void* data, uint32 size);
  bool SerializerEnd     (BinaryReader* serializer);
  void SerializerRead    (BinaryReader* serializer, void* data, uint32 size);
  void SerializerAttach  (BinaryReader* serializer, void** data, uint32 size);
};