//
// Copyright 2020 AnimalLogic
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
#include "trsOptions.h"

#include <utility>

#include <pxr/pxr.h>
#include <pxr/usd/ar/resolver.h>

#include <maya/MArgList.h>
#include <maya/MStatus.h>
#include <maya/MSyntax.h>
#include <maya/MSelectionList.h>

#include <mayaUsd/fileio/jobs/jobArgs.h>
#include <mayaUsd/fileio/jobs/readJob.h>
#include <mayaUsd/utils/util.h>
#include <mayaUsdUtils/TransformOpTools.h>

PXR_NAMESPACE_USING_DIRECTIVE

MAYAUSD_NS_DEF {


MStatus TRSOptions::doIt(const MArgList& args) 
{
    MStatus status;
    MArgDatabase db(TRSOptions::syntax(), args, &status);
    if(!status)
        return status;

    if(db.isFlagSet("-cl"))
    {
        MayaUsdUtils::TransformOpProcessor::primaryTranslateSuffix = 
        MayaUsdUtils::TransformOpProcessor::primaryScaleSuffix = 
        MayaUsdUtils::TransformOpProcessor::primaryRotateSuffix = TfToken();
    }

    if(db.isFlagSet("-tr"))
    {
        MString str;
        if(db.getFlagArgument("-tr", 0, str))
        {
            MayaUsdUtils::TransformOpProcessor::primaryTranslateSuffix = TfToken(str.asChar());
        }
    }
    
    if(db.isFlagSet("-ro"))
    {
        MString str;
        if(db.getFlagArgument("-ro", 0, str))
        {
            MayaUsdUtils::TransformOpProcessor::primaryRotateSuffix = TfToken(str.asChar());
        }
    }
    
    if(db.isFlagSet("-sc"))
    {
        MString str;
        if(db.getFlagArgument("-sc", 0, str))
        {
            MayaUsdUtils::TransformOpProcessor::primaryScaleSuffix = TfToken(str.asChar());
        }
    }
    return MS::kSuccess;
}

MSyntax TRSOptions::createSyntax()
{
    MSyntax syn;
    syn.addFlag("-tr", "-translate", MSyntax::kString);
    syn.addFlag("-ro", "-rotate", MSyntax::kString);
    syn.addFlag("-sc", "-scale", MSyntax::kString);
    syn.addFlag("-cl", "-clear");
    return syn;
}

void* TRSOptions::creator()
{
    return new TRSOptions;
}

}

