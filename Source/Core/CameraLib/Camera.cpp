//==============================================================================
// (c)2012 Joe Schutte
//==============================================================================

#include "Camera.h"
#include <RenderServer/Common/RsCamera.h>
#include <MathLib/FloatFuncs.h>
#include <SystemLib/CheckedCast.h>

namespace Shooty {

  //==============================================================================
  void camera_update_server_camera (Camera* camera, RsCamera* server_camera)
  {
    float aspect = itof_cast(camera->viewport.right - camera->viewport.left) / itof_cast(camera->viewport.bottom - camera->viewport.top);
    server_camera->viewport               = camera->viewport;  
    server_camera->view_pos               = camera->position;
    server_camera->view_matrix            = mat_lookat_lh(camera->position, camera->up, camera->focus);
    server_camera->projection_matrix      = mat_perspective_fov_lh(camera->fov, aspect, camera->znear, camera->zfar);
    server_camera->view_projection_matrix = mat_mul(server_camera->view_matrix, server_camera->projection_matrix);
    server_camera->fov                    = camera->fov;
    server_camera->znear                  = camera->znear;
    server_camera->zfar                   = camera->zfar;
    server_camera->aspect                 = aspect;
  }

};