//
// Copyright 2025 Autodesk
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
#include "layerContentsWidget.h"

#include "layerTreeModel.h"
#include "stringResources.h"
#include "usdSyntaxHighlighter.h"

#include <mayaUsd/base/tokens.h>
#include <mayaUsd/utils/util.h>

#include <pxr/base/tf/patternMatcher.h>
#include <pxr/base/tf/stringUtils.h>
#include <pxr/usd/sdf/copyUtils.h>
#include <pxr/usd/sdf/layer.h>
#include <pxr/usd/sdf/primSpec.h>
#include <pxr/usd/sdf/usdFileFormat.h>
#include <pxr/usd/sdf/usdaFileFormat.h>

#include <maya/MGlobal.h>

#include <string>
#include <utility>
#include <vector>

namespace UsdLayerEditor {

namespace OutputType {

PXR_NAMESPACE_USING_DIRECTIVE

// clang-format off

// This code was copied from Pixar OpenUSD's sdffilter.cpp, with some modifications to fit our use case.
// I disabled the clang-format for this section to preserve the original formatting of the code, which makes
// it easier to maintain when comparing to the original source.

// A file format for the human readable "pseudoLayer" output.  We use this so
// that the terse human-readable output we produce is not a valid layer nor may
// be mistaken for one.
class SdfFilterPseudoFileFormat : public SdfUsdaFileFormat
{
private:
    SDF_FILE_FORMAT_FACTORY_ACCESS;

public:
    SdfFilterPseudoFileFormat(std::string description="<< human readable >>")
        : SdfUsdaFileFormat(TfToken("pseudousda"),
                            TfToken(description),
                            SdfUsdFileFormatTokens->Target) {}
private:

    virtual ~SdfFilterPseudoFileFormat() {}
};

TF_REGISTRY_FUNCTION(TfType)
{
    SDF_DEFINE_FILE_FORMAT(SdfFilterPseudoFileFormat, SdfUsdaFileFormat);
}

// An enum representing the type of output to produce.
enum OutputType {
    OutputPseudoLayer,  // report as human readable text, as close to a
                        // valid layer as possible
    OutputLayer         // produce a valid layer as output.
};

// We use this structure to represent all the parameters for reporting. 
struct ReportParams
{
    std::shared_ptr<TfPatternMatcher> pathMatcher;
    std::shared_ptr<TfPatternMatcher> fieldMatcher;

    // Default to the most human readable output (pseudoLayer) which is the only
    // time we use ReportParams.
    OutputType outputType = OutputType::OutputPseudoLayer;

    int64_t arraySizeLimit = 8;
    int64_t timeSamplesSizeLimit = 8;
};

// Find all the paths in layer that match, or all paths if matcher is null.
std::vector<SdfPath>
CollectMatchingSpecPaths(SdfLayerHandle const &layer,
                         TfPatternMatcher const *matcher)
{
    std::vector<SdfPath> result;
    layer->Traverse(SdfPath::AbsoluteRootPath(),
                    [&result, &matcher](SdfPath const &path) {
                        if (!matcher || matcher->Match(path.GetString()))
                            result.push_back(path);
                    });
    return result;
}

// Closeness check with relative tolerance.
bool IsClose(double a, double b, double tol)
{
    double absDiff = fabs(a-b);
    return absDiff <= fabs(tol*a) || absDiff < fabs(tol*b);
}

// Get a suitable value for the report specified by p.  In particular, for
// non-layer output, make a value that shows only array type & size for large
// arrays.
VtValue
GetReportValue(VtValue const &value, ReportParams const &p)
{
    if (p.outputType != OutputLayer &&
        p.arraySizeLimit >= 0 &&
        value.IsArrayValued() &&
        value.GetArraySize() > static_cast<uint64_t>(p.arraySizeLimit)) {
        return VtValue(
            SdfHumanReadableValue(
                TfStringPrintf(
                    "%s[%zu]",
                    ArchGetDemangled(value.GetElementTypeid()).c_str(),
                    value.GetArraySize())));
    }
    return value;
}

// Get a suitable value for timeSamples for the report specified by p.  In
// particular, for non-layer output, make a value that shows number of samples
// and their time range.
VtValue
GetReportTimeSamplesValue(SdfLayerHandle const &layer,
                          SdfPath const &path, ReportParams const &p)
{
    auto times = layer->ListTimeSamplesForPath(path);
    std::vector<double> selectedTimes;
    selectedTimes.reserve(times.size());

    selectedTimes.assign(times.begin(), times.end());

    if (selectedTimes.empty())
        return VtValue();

    SdfTimeSampleMap result;
    
    VtValue val;
    if (p.outputType != OutputLayer &&
        p.timeSamplesSizeLimit >= 0 &&
        selectedTimes.size() > static_cast<uint64_t>(p.timeSamplesSizeLimit)) {
        return VtValue(
            SdfHumanReadableValue(
                TfStringPrintf("%zu samples in [%s, %s]",
                               times.size(),
                               TfStringify(*times.begin()).c_str(),
                               TfStringify(*(--times.end())).c_str())
                )
            );
    }
    else {
        for (auto time: selectedTimes) {
            TF_VERIFY(layer->QueryTimeSample(path, time, &val));
            result[time] = GetReportValue(val, p);
        }
    }
    return VtValue(result);
}

// Get a suitable value for the report specified by p.  In particular, for
// non-layer output, make a value that shows only array type & size for large
// arrays or number of time samples and time range for large timeSamples.
VtValue
GetReportFieldValue(SdfLayerHandle const &layer,
                           SdfPath const &path, TfToken const &field,
                           ReportParams const &p)
{
    VtValue result;
    // Handle timeSamples specially:
    if (field == SdfFieldKeys->TimeSamples) {
        result = GetReportTimeSamplesValue(layer, path, p);
    } else {
        TF_VERIFY(layer->HasField(path, field, &result));
        result = GetReportValue(result, p);
    }
    return result;
}

// Utility function to filter a layer by the params p.  This copies fields,
// replacing large arrays and timeSamples with human readable values if
// appropriate, and skipping paths and fields that do not match the matchers in
// p.
void
FilterLayer(SdfLayerHandle const &inLayer,
            SdfLayerHandle const &outLayer,
            ReportParams const &p)
{
    namespace ph = std::placeholders;
    auto copyValueFn = [&p](
        SdfSpecType specType, TfToken const &field,
        SdfLayerHandle const &srcLayer, const SdfPath& srcPath, bool fieldInSrc,
        const SdfLayerHandle& dstLayer, const SdfPath& dstPath, bool fieldInDst,
        std::optional<VtValue> *valueToCopy) {

        if (!p.fieldMatcher || p.fieldMatcher->Match(field.GetString())) {
            *valueToCopy = GetReportFieldValue(
                srcLayer, srcPath, field, p);
            return !(*valueToCopy)->IsEmpty();
        }
        else {
            return false;
        }
    };

    std::vector<SdfPath> paths =
        CollectMatchingSpecPaths(inLayer, p.pathMatcher.get());
    for (auto const &path: paths) {
        if (path == SdfPath::AbsoluteRootPath() ||
            path.IsPrimOrPrimVariantSelectionPath()) {
            SdfPrimSpecHandle outPrim = SdfCreatePrimInLayer(outLayer, path);
            SdfCopySpec(inLayer, path, outLayer, path,
                        copyValueFn,
                        std::bind(SdfShouldCopyChildren,
                                  std::cref(path), std::cref(path),
                                  ph::_1, ph::_2, ph::_3, ph::_4, ph::_5,
                                  ph::_6, ph::_7, ph::_8, ph::_9));
        }
    }
}

// clang-format on

} // namespace OutputType

struct SuspendUsdNotices
{
    SuspendUsdNotices() { LayerTreeModel::suspendUsdNotices(true); }
    ~SuspendUsdNotices() { LayerTreeModel::suspendUsdNotices(false); }
};

LayerContentsWidget::LayerContentsWidget(QWidget* in_parent)
    : QWidget(in_parent)
{
    createUI();
}

LayerContentsWidget::~LayerContentsWidget() { }

void LayerContentsWidget::createUI()
{
    auto mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    _layerContents = new QTextEdit;
    _layerContents->setFont(QFont("Courier New"));
    _layerContents->setAcceptRichText(true);
    _layerContents->setFrameStyle(QFrame::NoFrame);
    _layerContents->setPlaceholderText(
        StringResources::getAsQString(StringResources::kDisplayLayerContentsEmpty));
    _layerContents->setReadOnly(true);

    // Apply USD syntax highlighting
    _syntaxHighlighter = new UsdSyntaxHighlighter(_layerContents->document());

    mainLayout->addWidget(_layerContents);
    setLayout(mainLayout);
}

void LayerContentsWidget::setLayer(
    const PXR_NS::SdfLayerRefPtr in_layer,
    bool                         expandAllValues /*= false*/)
{
    if (_layerContents) {
        // Always clear the contents first as an input layer of nullptr means clear.
        // Note: that will be the case when there is no layer selected, or if there is
        //       more than one layer selected. We only display the contents of a layer
        //       when there is exactly one layer selected.
        _layerContents->clear();
        if (nullptr != in_layer) {
            std::string layerText;
            // Note: potentially slow operation for large layers.
            //       We could consider doing this in a worker thread if needed.

            bool layerExported { false };
            if (!expandAllValues) {
                layerExported = exportPseudoLayer(in_layer, layerText);
            } else {
                layerExported = in_layer->ExportToString(&layerText);
            }

            if (layerExported) {
                _layerContents->setPlainText(QString::fromStdString(layerText));
                _isEmpty = false;
            }
        }
    }
}

bool LayerContentsWidget::exportPseudoLayer(
    const PXR_NS::SdfLayerRefPtr in_layer,
    std::string&                 out_contents)
{
    // Suspend all notices while we do this, since the pseudo layer is not a real layer and may have
    // fields that would trigger notices that we don't want which cause the LE to rebuild.
    SuspendUsdNotices suspendNotices;

    // Make the pseudo layer and copy into it, then export.
    PXR_NS::SdfFileFormatConstRefPtr fmt;
    OutputType::ReportParams         params;

    // Check if the user wants to change any of the size limits.
    bool exists { false };
    int  opt = MGlobal::optionVarIntValue(
        UsdMayaUtil::convert(MayaUsdOptionVars->LayerContentsArraySizeLimit), &exists);
    if (exists) {
        params.arraySizeLimit = opt;
    }
    opt = MGlobal::optionVarIntValue(
        UsdMayaUtil::convert(MayaUsdOptionVars->LayerContentsTimeSamplesSizeLimit), &exists);
    if (exists) {
        params.timeSamplesSizeLimit = opt;
    }

    fmt = PXR_NS::TfCreateRefPtr(new OutputType::SdfFilterPseudoFileFormat(
        PXR_NS::TfStringPrintf("from @%s@", in_layer->GetIdentifier().c_str())));
    auto pseudoLayer = PXR_NS::SdfLayer::CreateAnonymous(".pseudousda", fmt);

    // Generate the layer content.
    OutputType::FilterLayer(in_layer, pseudoLayer, params);

    return pseudoLayer->ExportToString(&out_contents);
}

void LayerContentsWidget::clear()
{
    if (_layerContents) {
        _layerContents->clear();
        _isEmpty = true;
    }
}

} // namespace UsdLayerEditor
