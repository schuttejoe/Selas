#pragma once

//==============================================================================
// (c)2012 Joe Schutte
//==============================================================================

#include <MathLib/FloatStructs.h>
#include <ContainersLib/Rect.h>

namespace Shooty {

  struct RsCamera;

  //==============================================================================
  struct Camera
  {
    Rect   viewport;
    float3 position;
    float3 focus;
    float3 up;

    float fov;
    float znear;
    float zfar;
  };

  void camera_update_server_camera (Camera* camera, RsCamera* server_camera);
};