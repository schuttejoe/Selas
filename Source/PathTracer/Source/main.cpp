
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


    __declspec(align(16)) RTCRay ray;
    ray.org[0] = orig.x;
    ray.org[1] = orig.y;
    ray.org[2] = orig.z;

    ray.dir[0] = direction.x;
    ray.dir[1] = direction.y;
    ray.dir[2] = direction.z;

    ray.tnear = znear;
    ray.tfar = zfar;

    ray.time = 0.0f;
    ray.mask = 0;

    ray.geomID = -1;
    ray.primID = -1;
    ray.instID = -1;

    rtcIntersect(rtcScene, ray);

    position.x = orig.x + ray.tfar * direction.x;
    position.y = orig.y + ray.tfar * direction.y;
    position.z = orig.z + ray.tfar * direction.z;
    baryCoords = { ray.u, ray.v };
    primId = ray.primID;
    
    return ray.geomID != -1;
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
static float3 ImportanceSampleIbl(ImageBasedLightResourceData* ibl, Random::MersenneTwister* twister, float3 n, float3 v, float3 albedo, float3 reflectance, float roughness)
{

    const uint sampleCount = 1024;

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

        float3 sample = ibl->hdrData[y * ibl->densityfunctions.width + x];
        float3 irradiance = GgxBrdf(n, wi, v, albedo, reflectance, roughness);

        lighting += sample * irradiance * weight;
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
    float znear = 0.1f;
    float zfar = 500.0f;
    float fov = 40.0f * Math::DegreesToRadians_;
    // dragon
    //float3 cameraPosition = float3(0.0f, 7.0f, 20.0f);
    //float3 lookAt = float3(0.0f, 5.0f, 0.0f);
    // sphere
    //float3 cameraPosition = float3(0.0f, 0.0f, 4.0f);
    //float3 lookAt = float3(0.0f, 0.0f, 0.0f);
    // teapot
    //float3 cameraPosition = float3(-20.0f, 0.0f, 45.0f);
    //float3 lookAt = float3(0.0f, 0.0f, 0.0f);
    // bunny
    float3 cameraPosition = float3(-1.0f, 1.5f, -3.0f);
    float3 lookAt = float3(-0.2f, 0.8f, 0.0f);

    float4x4 projection = PerspectiveFovLhProjection(fov, aspect, znear, zfar);
    float4x4 view = LookAtLh(cameraPosition, float3::YAxis_, lookAt);
    float4x4 viewProj = MatrixMultiply(view, projection);

    RayCastCameraSettings camera;
    camera.invViewProjection = MatrixInverse(viewProj);
    camera.viewport = {0, 0, (sint)width, (sint)height};
    camera.position = cameraPosition;
    camera.znear = znear;
    camera.zfar = zfar;

    uint32* imageData = AllocArray_(uint32, width * height);
    
    const float iblIntensity = 1.0f;
    const float3 lightIntensity = 0.0f * float3(226.0f/255.0f, 197.0f/255.0f, 168.0f/255.0f);
    const float3 lightPosition = float3(15.0f, 30.0f, 45.0f);
    const float roughness = 0.2f;
    const float3 reflectance = float3(0.8f, 0.8f, 0.8f);
    const float3 albedo = float3(0.01f, 0.01f, 0.01f);

    for (uint y = 0; y < height; ++y) {
        for (uint x = 0; x < width; ++x) {

            float3 direction = MakeCameraRayDirection(&camera, x, y, width, height);

            float3 position;
            float2 baryCoords;
            int32 primId;
            bool hit = RayPick(rtcScene, scene, cameraPosition, direction, znear, zfar, position, baryCoords, primId);

            float3 color = float3::Zero_;

            if(hit) {
                float3 n;
                float2 uvs;
                CalculateNormalAndUvs(scene, primId, baryCoords, n, uvs);

                float3 v = Normalize(cameraPosition - position);
                
                // -- kinda hacked ibl
                float3 iblContrib = iblIntensity * ImportanceSampleIbl(ibl, &twister, n, v, albedo, reflectance, roughness);
                
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
    int retvalue = 0;

    RTCDevice rtcDevice = rtcNewDevice(nullptr);
    RTCScene rtcScene = rtcDeviceNewScene(rtcDevice, RTC_SCENE_STATIC, RTC_INTERSECT1 | RTC_INTERSECT8);
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

    meshHandle = PopulateEmbreeScene(sceneResource.data, rtcDevice, rtcScene);
    GenerateRayCastImage(rtcScene, sceneResource.data, iblResouce.data, 1024, 1024);

cleanup:
    // -- delete the scene
    SafeFree_(sceneResource.data);
    SafeFree_(iblResouce.data);

    if(meshHandle != -1) {
        rtcDeleteGeometry(rtcScene, meshHandle);
    }
    rtcDeleteScene(rtcScene);
    rtcDeleteDevice(rtcDevice);

    return retvalue;
}