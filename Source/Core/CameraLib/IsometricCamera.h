#pragma once

//==============================================================================
// (c)2012 Joe Schutte
//==============================================================================

#include <RenderServer\Common\RsHandle.h>
#include <IoLib\XInputControllers.h>
#include <MathLib\FloatStructs.h>
#include <SystemLib\BasicTypes.h>

namespace Shooty {

  struct Camera;

  //==============================================================================
  struct IsometricCameraController
  {
    float distance;
    float theta;
    float phi;
  };

  void compute_spherical_coordinates_y_up (const float3& position, const float3& focus, float& distance, float& theta, float& phi);
  void isometric_camera_init   (IsometricCameraController* controller, float distance, float theta, float phi);
  void isometric_camera_update (IsometricCameraController* controller, Camera* camera);
  void isometric_cameta_compute_movement_vectors (Camera* camera, float3& forward, float3& right);
};