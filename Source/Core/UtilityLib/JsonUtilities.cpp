//=================================================================================================================================
// Joe Schutte
//=================================================================================================================================

#include "UtilityLib/JsonUtilities.h"
#include "IoLib/File.h"
#include "SystemLib/MemoryAllocation.h"

// -- middleware
#include "rapidjson/document.h"

namespace Selas
{
    namespace Json
    {
        //=========================================================================================================================
        Error OpenJsonDocument(cpointer filepath, rapidjson::Document& document)
        {
            char* jsonString;
            uint32 jsonStringSize;
            ReturnError_(File::ReadWhileFileAsString(filepath, &jsonString, &jsonStringSize));

            rapidjson::ParseResult result = document.Parse(jsonString);
            FreeAligned_(jsonString);

            if(result.IsError()) {
                return Error_("Json parsing of %s failed with code %u", filepath, result.Code());
            }

            return Success_;
        }

        //=========================================================================================================================
        bool IsStringAttribute(const rapidjson::Value& element, cpointer key)
        {
            if(element.HasMember(key) == false) {
                return false;
            }

            if(element[key].IsString() == false) {
                return false;
            }

            return true;
        }

        //=========================================================================================================================
        bool IsFloatAttribute(const rapidjson::Value& element, cpointer key)
        {
            if(element.HasMember(key) == false) {
                return false;
            }

            if(element[key].IsFloat() == false) {
                return false;
            }

            return true;
        }

        //=========================================================================================================================
        bool IsFloat3Attribute(const rapidjson::Value& element, cpointer key)
        {
            if(element[key].IsArray() && element[key].Size() == 2) {
                if(element[key][0].IsFloat() == false) {
                    return false;
                }
                if(element[key][1].IsFloat() == false) {
                    return false;
                }
                if(element[key][2].IsFloat() == false) {
                    return false;
                }

                return true;
            }

            return false;
        }

        //=========================================================================================================================
        bool ReadBool(const rapidjson::Value& element, cpointer key, bool& value, bool defaultValue)
        {
            if(element.HasMember(key)) {
                if(element[key].IsBool()) {
                    value = element[key].GetBool();
                    return true;
                }
                else {
                    value = defaultValue;
                    return false;
                }
            }
            else {
                value = defaultValue;
                return false;
            }
        }

        //=========================================================================================================================
        bool ReadInt32(const rapidjson::Value& element, cpointer key, int32& value, int32 defaultValue)
        {
            if(element.HasMember(key)) {
                if(element[key].IsInt()) {
                    value = element[key].GetInt();
                    return true;
                }
                else {
                    value = defaultValue;
                    return false;
                }
            }
            else {
                value = defaultValue;
                return false;
            }
        }

        //=========================================================================================================================
        bool ReadFloat(const rapidjson::Value& element, cpointer key, float& value, float defaultValue)
        {
            if(element.HasMember(key)) {
                if(element[key].IsFloat()) {
                    value = element[key].GetFloat();
                    return true;
                }
                else {
                    value = defaultValue;
                    return false;
                }
            }
            else {
                value = defaultValue;
                return false;
            }
        }

        //=========================================================================================================================
        bool ReadFloat2(const rapidjson::Value& element, cpointer key, float2& value, float2 defaultValue)
        {
            if(element.HasMember(key)) {
                if(element[key].IsArray() && element[key].Size() == 2) {
                    value.x = element[key][0].GetFloat();
                    value.y = element[key][1].GetFloat();
                    return true;
                }
                else {
                    value = defaultValue;
                    return false;
                }
            }
            else {
                value = defaultValue;
                return false;
            }
        }

        //=========================================================================================================================
        bool ReadFloat3(const rapidjson::Value& element, cpointer key, float3& value, float3 defaultValue)
        {
            if(element.HasMember(key)) {
                if(element[key].IsArray() && element[key].Size() == 3) {
                    value.x = element[key][0].GetFloat();
                    value.y = element[key][1].GetFloat();
                    value.z = element[key][2].GetFloat();
                    return true;
                }
                else {
                    value = defaultValue;
                    return false;
                }
            }
            else {
                value = defaultValue;
                return false;
            }
        }

        //=========================================================================================================================
        bool ReadFloat4(const rapidjson::Value& element, cpointer key, float4& value, float4 defaultValue)
        {
            if(element.HasMember(key)) {
                if(element[key].IsArray() && element[key].Size() == 4) {
                    value.x = element[key][0].GetFloat();
                    value.y = element[key][1].GetFloat();
                    value.z = element[key][2].GetFloat();
                    value.w = element[key][3].GetFloat();
                    return true;
                }
                else {
                    value = defaultValue;
                    return false;
                }
            }
            else {
                value = defaultValue;
                return false;
            }
        }
    }
}