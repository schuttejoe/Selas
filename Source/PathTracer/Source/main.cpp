
//==============================================================================
// Joe Schutte
//==============================================================================

#include <SceneLib/SceneResource.h>
#include <UtilityLib/StbImageWrite.h>
#include <MathLib/FloatFuncs.h>
#include <MathLib/FloatStructs.h>
#include <MathLib/Trigonometric.h>
#include <ContainersLib/Rect.h>
#include <SystemLib/MemoryAllocation.h>
#include <SystemLib/BasicTypes.h>
#include <SystemLib/Memory.h>

#include <embree2/rtcore.h>
#include <embree2/rtcore_ray.h>

using namespace Shooty;

struct EmbreeVertex   { float x, y, z, a; };
struct EmbreeTriangle { int i0, i1, i2; };

//==============================================================================
struct RayCastCameraSettings {
    float4x4 invViewProjection;
    Rect     viewport;
    float3   position;
    float    znear;
    float    zfar;
};

//==============================================================================
static uint32 PopulateEmbreeScene(SceneResourceData* sceneData, RTCDevice& rtcDevice, RTCScene& rtcScene) {

    uint32 vertexCount = sceneData->totalVertexCount;
    uint32 indexCount = sceneData->totalIndexCount;
    uint32 triangleCount = indexCount / 3;

    uint32 rtcMeshHandle = rtcNewTriangleMesh(rtcScene, RTC_GEOMETRY_STATIC, triangleCount, vertexCount);

    EmbreeTriangle* triangles = (EmbreeTriangle*)rtcMapBuffer(rtcScene, rtcMeshHandle, RTC_INDEX_BUFFER);
    Memory::Copy(triangles, sceneData->indices, sceneData->totalIndexCount * sizeof(uint32));
    rtcUnmapBuffer(rtcScene, rtcMeshHandle, RTC_INDEX_BUFFER);

    EmbreeVertex* vertices = (EmbreeVertex*)rtcMapBuffer(rtcScene, rtcMeshHandle, RTC_VERTEX_BUFFER);
    Memory::Copy(vertices, sceneData->positions, sceneData->totalVertexCount * sizeof(EmbreeVertex));
    rtcUnmapBuffer(rtcScene, rtcMeshHandle, RTC_VERTEX_BUFFER);

    rtcCommit(rtcScene);

    return rtcMeshHandle;
}

//==============================================================================
static bool RayPick(RTCScene& rtcScene, RayCastCameraSettings* __restrict camera, uint viewX, uint viewY, uint width, uint height) {
    float x = (2.0f * viewX) / width - 1.0f;
    float y = 1.0f - (2.0f * viewY) / height;

    float4 un = MatrixMultiplyFloat4(float4(x, y, 0.0f, 1.0f), camera->invViewProjection);
    float3 worldPosition = (1.0f / un.w) * float3(un.x, un.y, un.z);
    float3 direction = Normalize(worldPosition - camera->position);

    __declspec(align(16)) RTCRay ray;
    ray.org[0] = camera->position.x;
    ray.org[1] = camera->position.y;
    ray.org[2] = camera->position.z;

    ray.dir[0] = direction.x;
    ray.dir[1] = direction.y;
    ray.dir[2] = direction.z;

    ray.tnear = camera->znear;
    ray.tfar = camera->zfar;

    ray.time = 0.0f;
    ray.mask = 0;

    ray.geomID = -1;
    ray.primID = -1;
    ray.instID = -1;

    rtcIntersect(rtcScene, ray);

    return ray.geomID != -1;
}

//==============================================================================
static void GenerateRayCastImage(RTCScene& rtcScene, uint width, uint height) {

    float aspect = (float)width / height;
    float znear = 0.1f;
    float zfar = 500.0f;
    float fov = 40.0f * Math::DegreesToRadians_;
    float3 cameraPosition = float3(15.0f, 0.0f, -45.0f);

    float4x4 projection = PerspectiveFovLhProjection(fov, aspect, znear, zfar);
    float4x4 view = LookAtLh(cameraPosition, float3::YAxis_, float3::Zero_);
    float4x4 viewProj = MatrixMultiply(view, projection);

    RayCastCameraSettings camera;
    camera.invViewProjection = MatrixInverse(viewProj);
    camera.viewport = {0, 0, (sint)width, (sint)height};
    camera.position = cameraPosition;
    camera.znear = znear;
    camera.zfar = zfar;

    uint32* imageData = AllocArray_(uint32, width * height);
    
    for (uint y = 0; y < height; ++y) {
        for (uint x = 0; x < width; ++x) {
            bool hit = RayPick(rtcScene, &camera, x, y, width, height);

            imageData[y * width + x] = hit ? 0xFFFFFFFF : 0;
        }
    }

    StbImageWrite("D:\\temp\\test.png", width, height, PNG, imageData);

    Free_(imageData);
}

//==============================================================================
int main()
{
    int retvalue = 0;

    RTCDevice rtcDevice = rtcNewDevice(nullptr);
    RTCScene rtcScene = rtcDeviceNewScene(rtcDevice, RTC_SCENE_STATIC, RTC_INTERSECT1 | RTC_INTERSECT8);
    uint32 meshHandle = -1;

    SceneResource sceneResource;
    if (ReadSceneResource("D:\\Shooty\\ShootyEngine\\_Assets\\scene.bin", &sceneResource) == false) {
        retvalue = -1;
        goto cleanup;
    }

    meshHandle = PopulateEmbreeScene(sceneResource.data, rtcDevice, rtcScene);
    GenerateRayCastImage(rtcScene, 1024, 1024);

cleanup:
    if (meshHandle != -1) {
        rtcDeleteGeometry(rtcScene, meshHandle);
    }
    rtcDeleteScene(rtcScene);
    rtcDeleteDevice(rtcDevice);

	return retvalue;
}