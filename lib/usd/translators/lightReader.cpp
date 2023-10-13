//
// Copyright 2021 Autodesk
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
#include <mayaUsd/fileio/primReaderRegistry.h>
#include <mayaUsd/fileio/translators/translatorLight.h>
#include <mayaUsd/fileio/translators/translatorRfMLight.h>

#include <pxr/base/tf/envSetting.h>

PXR_NAMESPACE_OPEN_SCOPE

TF_DEFINE_ENV_SETTING(
    MAYAUSD_IMPORT_RFM_LIGHTS,
    false,
    "Whether to import UsdLux as Renderman-for-Maya lights.");

PXRUSDMAYA_DEFINE_READER(UsdLuxCylinderLight, args, context)
{
    if (TfGetEnvSetting(MAYAUSD_IMPORT_RFM_LIGHTS)) {
        return UsdMayaTranslatorRfMLight::Read(args, context);
    }
    return false;
}

PXRUSDMAYA_DEFINE_READER(UsdLuxDiskLight, args, context)
{
    if (TfGetEnvSetting(MAYAUSD_IMPORT_RFM_LIGHTS)) {
        return UsdMayaTranslatorRfMLight::Read(args, context);
    }
    return false;
}

PXRUSDMAYA_DEFINE_READER(UsdLuxDistantLight, args, context)
{
    if (TfGetEnvSetting(MAYAUSD_IMPORT_RFM_LIGHTS)) {
        return UsdMayaTranslatorRfMLight::Read(args, context);
    }
    return UsdMayaTranslatorLight::Read(args, context);
}

PXRUSDMAYA_DEFINE_READER(UsdLuxDomeLight, args, context)
{
    if (TfGetEnvSetting(MAYAUSD_IMPORT_RFM_LIGHTS)) {
        return UsdMayaTranslatorRfMLight::Read(args, context);
    }
    return false;
}

PXRUSDMAYA_DEFINE_READER(UsdLuxGeometryLight, args, context)
{
    if (TfGetEnvSetting(MAYAUSD_IMPORT_RFM_LIGHTS)) {
        return UsdMayaTranslatorRfMLight::Read(args, context);
    }
    return false;
}

PXRUSDMAYA_DEFINE_READER(UsdLuxRectLight, args, context)
{
    if (TfGetEnvSetting(MAYAUSD_IMPORT_RFM_LIGHTS)) {
        return UsdMayaTranslatorRfMLight::Read(args, context);
    }
    return UsdMayaTranslatorLight::Read(args, context);
}

PXRUSDMAYA_DEFINE_READER(UsdLuxSphereLight, args, context)
{
    if (TfGetEnvSetting(MAYAUSD_IMPORT_RFM_LIGHTS)) {
        return UsdMayaTranslatorRfMLight::Read(args, context);
    }
    return UsdMayaTranslatorLight::Read(args, context);
}

// Moving to use PXRUSDMAYA_DEFINE_READER_FOR_USD_TYPE in anticipation of
// codeless schemas for UsdRi types to be available soon!
PXRUSDMAYA_DEFINE_READER_FOR_USD_TYPE(PxrAovLight, args, context)
{
    if (TfGetEnvSetting(MAYAUSD_IMPORT_RFM_LIGHTS)) {
        return UsdMayaTranslatorRfMLight::Read(args, context);
    }
    return false;
}

// Moving to use PXRUSDMAYA_DEFINE_READER_FOR_USD_TYPE in anticipation of
// codeless schemas for UsdRi types to be available soon!
PXRUSDMAYA_DEFINE_READER_FOR_USD_TYPE(PxrEnvDayLight, args, context)
{
    if (TfGetEnvSetting(MAYAUSD_IMPORT_RFM_LIGHTS)) {
        return UsdMayaTranslatorRfMLight::Read(args, context);
    }
    return false;
}

PXR_NAMESPACE_CLOSE_SCOPE
