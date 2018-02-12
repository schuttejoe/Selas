
//==============================================================================
// Joe Schutte
//==============================================================================

#include "Brdf.h"

#include <SceneLib/SceneResource.h>
#include <SceneLib/ImageBasedLightResource.h>
#include <UtilityLib/StbImageWrite.h>
#include <UtilityLib/Color.h>
#include <MathLib/FloatFuncs.h>
#include <MathLib/FloatStructs.h>
#include <MathLib/Trigonometric.h>
#include <MathLib/ImportanceSampling.h>
#include <MathLib/Random.h>
#include <MathLib/Projection.h>
#include <MathLib/Quaternion.h>
#include <ContainersLib/Rect.h>
#include <StringLib/FixedString.h>
#include <SystemLib/MemoryAllocation.h>
#include <SystemLib/BasicTypes.h>
#include <SystemLib/Memory.h>
#include <SystemLib/SystemTime.h>

#include <embree3/rtcore.h>
#include <embree3/rtcore_ray.h>

#include "xmmintrin.h"
#include "pmmintrin.h"
#include <stdio.h>
#include <windows.h>

using namespace Shooty;

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

    RTCGeometry rtcMeshHandle = rtcNewGeometry(rtcDevice, RTC_GEOMETRY_TYPE_TRIANGLE);
   
    rtcSetSharedGeometryBuffer(rtcMeshHandle, RTC_BUFFER_TYPE_VERTEX, 0, RTC_FORMAT_FLOAT3, sceneData->positions, 0, sizeof(float3), sceneData->totalVertexCount);
    rtcSetSharedGeometryBuffer(rtcMeshHandle, RTC_BUFFER_TYPE_INDEX, 0, RTC_FORMAT_UINT3, sceneData->indices, 0, 3 * sizeof(uint32), sceneData->totalIndexCount / 3);
    rtcCommitGeometry(rtcMeshHandle);

    unsigned int geomID = rtcAttachGeometry(rtcScene, rtcMeshHandle);
    rtcReleaseGeometry(rtcMeshHandle);

    rtcCommitScene(rtcScene);

    return geomID;
}

//==============================================================================
static void CalculateNormalAndUvs(SceneResourceData* __restrict scene, uint32 primitiveId, float2 barycentric, float3& normal, float2& uvs)
{
    uint32 i0 = scene->indices[3 * primitiveId + 0];
    uint32 i1 = scene->indices[3 * primitiveId + 1];
    uint32 i2 = scene->indices[3 * primitiveId + 2];

    float3 n0 = scene->normals[i0];
    float3 n1 = scene->normals[i1];
    float3 n2 = scene->normals[i2];

    float2 t0 = float2::Zero_;//scene->uv0[i0];
    float2 t1 = float2::Zero_;//scene->uv0[i1];
    float2 t2 = float2::Zero_;//scene->uv0[i2];

    float b0 = Saturate(1.0f - (barycentric.x + barycentric.y));
    float b1 = barycentric.x;
    float b2 = barycentric.y;

    normal = b0 * n0 + b1 * n1 + b2 * n2;
    uvs    = b0 * t0 + b1 * t1 + b2 * t2;
}

//==============================================================================
static float3 MakeCameraRayDirection(RayCastCameraSettings* __restrict camera, uint viewX, uint viewY, uint width, uint height) {
    float x = (2.0f * viewX) / width - 1.0f;
    float y = 1.0f - (2.0f * viewY) / height;

    float4 un = MatrixMultiplyFloat4(float4(x, y, 0.0f, 1.0f), camera->invViewProjection);
    float3 worldPosition = (1.0f / un.w) * float3(un.x, un.y, un.z);

    return Normalize(worldPosition - camera->position);
}

//==============================================================================
static bool RayPick(RTCScene& rtcScene, SceneResourceData* scene, float3 orig, float3 direction, float znear, float zfar,
                    float3& position, float2& baryCoords, int32& primId) {

    RTCIntersectContext context;
    rtcInitIntersectContext(&context);
    
    __declspec(align(16)) RTCRayHit rayhit;
    rayhit.ray.org_x = orig.x;
    rayhit.ray.org_y = orig.y;
    rayhit.ray.org_z = orig.z;
    rayhit.ray.dir_x = direction.x;
    rayhit.ray.dir_y = direction.y;
    rayhit.ray.dir_z = direction.z;
    rayhit.ray.tnear = znear;
    rayhit.ray.tfar = zfar;

    rayhit.hit.geomID = RTC_INVALID_GEOMETRY_ID;
    rayhit.hit.primID = RTC_INVALID_GEOMETRY_ID;

    rtcIntersect1(rtcScene, &context, &rayhit);

    position.x = rayhit.ray.org_x + rayhit.ray.tfar * direction.x;
    position.y = rayhit.ray.org_y + rayhit.ray.tfar * direction.y;
    position.z = rayhit.ray.org_z + rayhit.ray.tfar * direction.z;
    baryCoords = { rayhit.hit.u, rayhit.hit.v };
    primId = rayhit.hit.primID;
    
    return rayhit.hit.geomID != -1;
}

//==============================================================================
static bool OcclusionRay(RTCScene& rtcScene, float3 orig, float3 direction, float znear, float zfar)
{
    RTCIntersectContext context;
    rtcInitIntersectContext(&context);

    __declspec(align(16)) RTCRay ray;
    ray.org_x = orig.x;
    ray.org_y = orig.y;
    ray.org_z = orig.z;
    ray.dir_x = direction.x;
    ray.dir_y = direction.y;
    ray.dir_z = direction.z;
    ray.tnear = znear;
    ray.tfar = zfar;

    rtcOccluded1(rtcScene, &context, &ray);

    // -- ray.tfar == -inf when hit occurs
    return (ray.tfar >= 0.0f);
}

//------------------------------------------------------------------------------
static float Attenuation(float distance)
{
    const float lightRange = 1000.0f;
    const float virtualRadius = 1.0f; // fake light is fake

    float linear = 2.0f / virtualRadius;
    float quadratic = 1.0f / (virtualRadius * virtualRadius);
    float shift = 1.0f / (1.0f + lightRange * (linear + lightRange * quadratic));

    return Saturate((1.0f / (1.0f + distance * (linear + distance * quadratic))) - shift);
}

//==============================================================================
static float3 ImportanceSampleIbl(RTCScene& rtcScene, ImageBasedLightResourceData* ibl, Random::MersenneTwister* twister, float3 p, float3 n, float3 v, float3 albedo, float3 reflectance, float roughness)
{
    const uint sampleCount = 512;

    float3 lighting = float3::Zero_;

    for(uint scan = 0; scan < sampleCount; ++scan) {
        float r0 = Random::MersenneTwisterFloat(twister);
        float r1 = Random::MersenneTwisterFloat(twister);

        float theta;
        float phi;
        uint x;
        uint y;
        float weight;
        ImportanceSampling::Ibl(&ibl->densityfunctions, r0, r1, theta, phi, x, y, weight);

        float3 wi = Math::SphericalToCartesian(theta, phi);
        if(Dot(wi, n) <= 0.0f)
            continue;

        if(OcclusionRay(rtcScene, p, wi, 0.001f, FloatMax_)) {
            float3 sample = ibl->hdrData[y * ibl->densityfunctions.width + x];
            float3 irradiance = GgxBrdf(n, wi, v, albedo, reflectance, roughness);

            lighting += sample * irradiance * weight;
        }
    }

    return lighting * (1.0f / sampleCount);
}

//==============================================================================
static float3 ImportanceSampleGgx(ImageBasedLightResourceData* ibl, Random::MersenneTwister* twister, float3 n, float3 v, float3 albedo, float3 reflectance, float roughness)
{
    const uint sampleCount = 1024;

    float3 lighting = float3::Zero_;

    float3 axis = Normalize(Cross(float3::YAxis_, n));
    float radians = Math::Acosf(Dot(float3::YAxis_, n));

    float4 q = Math::Quaternion::Create(radians, axis);

    for(uint scan = 0; scan < sampleCount; ++scan) {
        float r0 = Random::MersenneTwisterFloat(twister);
        float r1 = Random::MersenneTwisterFloat(twister);

        float theta;
        float phi;
        float weight;
        ImportanceSampling::Ggx(roughness, r0, r1, theta, phi, weight);

        float3 wi = Math::SphericalToCartesian(theta, phi);
        wi = Math::Quaternion::Rotate(q, wi);
        if(Dot(wi, n) <= 0.0f)
            continue;

        Math::NormalizedCartesianToSpherical(wi, theta, phi);

        uint x = (uint)((phi / Math::TwoPi_) * ibl->densityfunctions.width - 0.5f);
        uint y = (uint)((theta / Math::Pi_) * ibl->densityfunctions.height - 0.5f);
        Assert_(x < ibl->densityfunctions.width);
        Assert_(y < ibl->densityfunctions.height);

        float3 sample = ibl->hdrData[y * ibl->densityfunctions.width + x];
        float3 irradiance = GgxBrdf(n, wi, v, albedo, reflectance, roughness);

        lighting += sample * irradiance * weight;
    }

    return lighting * (1.0f / sampleCount);
}

//==============================================================================
static void GenerateRayCastImage(RTCScene& rtcScene, SceneResourceData* scene, ImageBasedLightResourceData* ibl, uint width, uint height)
{
    Random::MersenneTwister twister;
    Random::MersenneTwisterInitialize(&twister, 0);

    float aspect = (float)width / height;

    float4x4 projection = PerspectiveFovLhProjection(scene->camera.fov, aspect, scene->camera.znear, scene->camera.zfar);
    float4x4 view = LookAtLh(scene->camera.position, float3::YAxis_, scene->camera.lookAt);
    float4x4 viewProj = MatrixMultiply(view, projection);

    RayCastCameraSettings camera;
    camera.invViewProjection = MatrixInverse(viewProj);
    camera.viewport = {0, 0, (sint)width, (sint)height};
    camera.position = scene->camera.position;
    camera.znear = scene->camera.znear;
    camera.zfar = scene->camera.zfar;

    uint32* imageData = AllocArray_(uint32, width * height);
    
    const float iblIntensity = 1.0f;
    const float3 lightIntensity = 0.0f * float3(226.0f/255.0f, 197.0f/255.0f, 168.0f/255.0f);
    const float3 lightPosition = float3(15.0f, 30.0f, 45.0f);
    const float roughness = 0.7f;
    const float3 reflectance = float3(0.1f, 0.1f, 0.1f);
    const float3 albedo = float3(0.8f, 0.8f, 0.8f);

    for (uint y = 0; y < height; ++y) {
        for (uint x = 0; x < width; ++x) {

            float3 direction = MakeCameraRayDirection(&camera, x, y, width, height);

            float3 position;
            float2 baryCoords;
            int32 primId;
            bool hit = RayPick(rtcScene, scene, scene->camera.position, direction, scene->camera.znear, scene->camera.zfar, position, baryCoords, primId);

            float3 color = float3::Zero_;

            if(hit) {
                float3 n;
                float2 uvs;
                CalculateNormalAndUvs(scene, primId, baryCoords, n, uvs);

                float3 v = Normalize(scene->camera.position - position);
                
                // -- kinda hacked ibl
                float3 iblContrib = iblIntensity * ImportanceSampleIbl(rtcScene, ibl, &twister, position, n, v, albedo, reflectance, roughness);
                
                color = iblContrib;
            }

            imageData[y * width + x] = ColorRGBA(float4(color, 1.0f));
        }
    }

    Random::MersenneTwisterShutdown(&twister);

    StbImageWrite("D:\\temp\\test.png", width, height, PNG, imageData);

    Free_(imageData);
}

//==============================================================================
int main()
{
    _MM_SET_FLUSH_ZERO_MODE(_MM_FLUSH_ZERO_ON);
    _MM_SET_DENORMALS_ZERO_MODE(_MM_DENORMALS_ZERO_ON);

    int retvalue = 0;

    RTCDevice rtcDevice = rtcNewDevice(nullptr/*"verbose=3"*/);
    RTCScene rtcScene = rtcNewScene(rtcDevice);
    uint32 meshHandle = -1;

    SceneResource sceneResource;
    if(ReadSceneResource("D:\\Shooty\\ShootyEngine\\_Assets\\scene.bin", &sceneResource) == false) {
        retvalue = -1;
        goto cleanup;
    }

    ImageBasedLightResource iblResouce;
    if(ReadImageBasedLightResource("D:\\Shooty\\ShootyEngine\\_Assets\\ibl.bin", &iblResouce) == false) {
        retvalue = -1;
        goto cleanup;
    }

    int64 timer;

    SystemTime::GetCycleCounter(&timer);
    meshHandle = PopulateEmbreeScene(sceneResource.data, rtcDevice, rtcScene);
    float buildms = SystemTime::ElapsedMs(timer);

    uint width = 1280;
    uint height = 720;

    SystemTime::GetCycleCounter(&timer);
    GenerateRayCastImage(rtcScene, sceneResource.data, iblResouce.data, width, height);
    float renderms = SystemTime::ElapsedMs(timer);

    FixedString64 buildlog;
    sprintf_s(buildlog.Ascii(), buildlog.Capcaity(), "Scene build time %fms\n", buildms);

    FixedString64 renderlog;
    sprintf_s(renderlog.Ascii(), renderlog.Capcaity(), "Scene render time %fms\n", renderms);

    OutputDebugStringA(buildlog.Ascii());
    OutputDebugStringA(renderlog.Ascii());

cleanup:
    // -- delete the scene
    SafeFreeAligned_(sceneResource.data);
    SafeFreeAligned_(iblResouce.data);

    rtcReleaseScene(rtcScene);
    rtcReleaseDevice(rtcDevice);

    return retvalue;
}