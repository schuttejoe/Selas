//==============================================================================
// (c)2012 Joe Schutte
//==============================================================================

#include "IsometricCamera.h"
#include <CameraLib/Camera.h>
#include <MathLib/Trigonometric.h>
#include <MathLib/FloatFuncs.h>

namespace Shooty {

  static const float3 y_axis = { 0.0f, 1.0f, 0.0f };
  static const float3 x_axis = { 1.0f, 0.0f, 0.0f };

  //==============================================================================
  void compute_spherical_coordinates_y_up (const float3& position, const float3& focus, float& distance, float& theta, float& phi)
  {
    float3 offset = position - focus;

    distance = Math::sqrtf(dot(offset, offset));
    theta = Math::acosf(offset.y / distance);
    phi = Math::atan2f(offset.z, offset.x);
  }

  //==============================================================================
  void isometric_camera_init   (IsometricCameraController* controller, float distance, float theta, float phi)
  {
    controller->distance          = distance;
    controller->theta             = theta;
    controller->phi               = phi;
  }

  //==============================================================================
  void isometric_camera_update (IsometricCameraController* controller, Camera* camera)
  {
    float3 camera_focus  = camera->focus;

    float3 offset;
    offset.x = controller->distance * Math::sinf(controller->theta) * Math::cosf(controller->phi);
    offset.y = controller->distance * Math::cosf(controller->theta);
    offset.z = controller->distance * Math::sinf(controller->theta) * Math::sinf(controller->phi);

    camera->position = camera_focus + offset;
    camera->focus    = camera_focus;
  }

  //==============================================================================
  void isometric_cameta_compute_movement_vectors (Camera* camera, float3& forward, float3& right)
  {
    // -- compute movement vectors
    float3 camera_forward = normalize(camera->focus - camera->position);
    float3 up_assumption  = Math::absf(dot(y_axis, camera_forward)) < 1.0f ? y_axis : x_axis;
    float3 camera_right   = normalize(cross(up_assumption, camera_forward));

    // -- project the right and forward vectors onto the xz plane to get the final movement vectors
    forward = normalize(camera_forward - make_float3(0.0f, camera_forward.y, 0.0f));
    right   = normalize(camera_right   - make_float3(0.0f, camera_right.y, 0.0f));
  }

};