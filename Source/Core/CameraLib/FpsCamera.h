#pragma once

//==============================================================================
// (c)2012 Joe Schutte
//==============================================================================

#include <RenderServer\Common\RsHandle.h>
#include <IoLib\XInputControllers.h>
#include <MathLib\FloatStructs.h>
#include <SystemLib\BasicTypes.h>

namespace Shooty {

  // JSTODO - hmmmmm.

  struct Camera;

  //==============================================================================
  struct FpsCameraController
  {
    float speed;
    float speed_boost_scale;
    float speed_drag_scale;
  };

  void fps_camera_init   (FpsCameraController* controller, float speed, float boost_scale, float drag_scale);
  void fps_camera_update (FpsCameraController* controller, Camera* camera, ControllersState& controllers, ControllerId id, float elasped_ms);
};