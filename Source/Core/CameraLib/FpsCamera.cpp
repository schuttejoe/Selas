//==============================================================================
// (c)2012 Joe Schutte
//==============================================================================

#include "FpsCamera.h"
#include <CameraLib/Camera.h>
#include <IoLib/XInputControllers.h>
#include <MathLib/FloatFuncs.h>
#include <MathLib/Quaternion.h>

namespace Shooty {

  static const float3 y_axis = { 0.0f, 1.0f, 0.0f };
  static const float3 x_axis = { 1.0f, 0.0f, 0.0f };

  //==============================================================================
  static void update_offset (const float3& offset, float3& pos, float3& focus)
  {
    pos = pos + offset;
    focus = focus + offset;
  }

  //==============================================================================
  static void fps_camera_rotate (const float3& axis, float radians, float3& pos, float3& focus)
  {
    float3 offset = focus - pos;
    float4 q = Math::quaternion_create(radians, axis);
    float3 newoffset = Math::quaternion_rotate(q, offset);
    focus = pos + newoffset;
  }

  //==============================================================================
  void fps_camera_init (FpsCameraController* controller, float speed, float boost_scale, float drag_scale)
  {
    controller->speed = speed;
    controller->speed_boost_scale = boost_scale;
    controller->speed_drag_scale = drag_scale;
  }

  //==============================================================================
  void fps_camera_update (FpsCameraController* controller, Camera* camera, ControllersState& controllers, ControllerId id, float elasped_ms)
  {
    float3 camera_eye    = camera->position;
    float3 camera_up     = camera->up;
    float3 camera_focus  = camera->focus;

    float3 forward       = normalize(camera_focus - camera_eye);
    float3 up_assumption = Math::absf(dot(y_axis, forward)) < 1.0f ? y_axis : x_axis;
    float3 right         = normalize(cross(up_assumption, forward));
    float3 up            = normalize(cross(forward, right));

    if (controllers.controller_connected[id]) {

      float speed = controller->speed * elasped_ms;
      if(controller_button_pressed(&controllers, id, ControllerButton_LeftBumper)) {
        speed *= controller->speed_boost_scale;
      }
      if(controller_button_pressed(&controllers, id, ControllerButton_RightBumper)) {
        speed *= controller->speed_drag_scale;
      }

      // left thumbstick
      float dr0 = controllers.controller_states[id].left_thumbstick_x * speed;
      float df0 = controllers.controller_states[id].left_thumbstick_y * speed;
      float3 offset0 = dr0 * right + df0 * forward;
      update_offset(offset0, camera_eye, camera_focus);

      // dpad
      float dl1 = controller_button_pressed(&controllers, id, ControllerButton_DpadLeft) ? speed : 0.0f;
      float dr1 = controller_button_pressed(&controllers, id, ControllerButton_DpadRight) ? speed : 0.0f;
      float du1 = controller_button_pressed(&controllers, id, ControllerButton_DpadUp) ? speed : 0.0f;
      float dd1 = controller_button_pressed(&controllers, id, ControllerButton_DpadDown) ? speed : 0.0f;
      float3 offset1 = -dl1 * right + dr1 * right + du1 * up + -dd1 * up;
      update_offset(offset1, camera_eye, camera_focus);

      // right thumbstick
      float dyaw = controllers.controller_states[id].right_thumbstick_x * 0.02f;
      float dpitch = -controllers.controller_states[id].right_thumbstick_y * 0.02f;
      fps_camera_rotate(up, dyaw, camera_eye, camera_focus);
      fps_camera_rotate(right, dpitch, camera_eye, camera_focus);

      camera_up = normalize(cross(forward, right));
    } else {
      camera_up = up;
    }

    camera->position  = camera_eye;
    camera->up        = camera_up;
    camera->focus     = camera_focus;
  }

};