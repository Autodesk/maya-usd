//
// Copyright 2026 Autodesk
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
#include <gtest/gtest.h>

#include <maya/MFnPlugin.h>
#include <maya/MGlobal.h>
#include <maya/MPxCommand.h>
#include <maya/MStatus.h>
#include <maya/MString.h>

#include <QtWidgets/QApplication>

#include <sstream>
#include <string>
#include <vector>

namespace {

// ── JsonResultCollector ───────────────────────────────────────────────────────

struct TestResult {
    std::string name;
    bool        passed;
    std::string message;
};

class JsonResultCollector : public ::testing::TestEventListener
{
public:
    void OnTestStart(const ::testing::TestInfo& info) override
    {
        _current.name    = std::string(info.test_suite_name()) + "." + info.name();
        _current.passed  = true;
        _current.message = "";
    }

    void OnTestPartResult(const ::testing::TestPartResult& result) override
    {
        if (!result.passed()) {
            _current.passed = false;
            if (!_current.message.empty())
                _current.message += "\n";
            _current.message += result.message();
        }
    }

    void OnTestEnd(const ::testing::TestInfo& /*info*/) override
    {
        _results.push_back(_current);
    }

    std::string toJson() const
    {
        std::ostringstream os;
        os << "[";
        for (size_t i = 0; i < _results.size(); ++i) {
            const auto& r = _results[i];
            if (i > 0) os << ",";
            os << "{\"name\":\"" << escape(r.name) << "\","
               << "\"passed\":" << (r.passed ? "true" : "false") << ","
               << "\"message\":\"" << escape(r.message) << "\"}";
        }
        os << "]";
        return os.str();
    }

    // Unused events — required by pure-virtual base.
    void OnTestProgramStart(const ::testing::UnitTest&) override          { }
    void OnTestIterationStart(const ::testing::UnitTest&, int) override   { }
    void OnEnvironmentsSetUpStart(const ::testing::UnitTest&) override    { }
    void OnEnvironmentsSetUpEnd(const ::testing::UnitTest&) override      { }
    void OnEnvironmentsTearDownStart(const ::testing::UnitTest&) override { }
    void OnEnvironmentsTearDownEnd(const ::testing::UnitTest&) override   { }
    void OnTestIterationEnd(const ::testing::UnitTest&, int) override     { }
    void OnTestProgramEnd(const ::testing::UnitTest&) override            { }

private:
    TestResult              _current;
    std::vector<TestResult> _results;

    static std::string escape(const std::string& s)
    {
        std::string out;
        out.reserve(s.size());
        for (char c : s) {
            switch (c) {
                case '"':  out += "\\\""; break;
                case '\\': out += "\\\\"; break;
                case '\n': out += "\\n";  break;
                case '\r': out += "\\r";  break;
                case '\t': out += "\\t";  break;
                default:   out += c;
            }
        }
        return out;
    }
};

// ── MPxCommand ────────────────────────────────────────────────────────────────

class RunLayerEditorTestsCmd : public MPxCommand
{
public:
    static const MString kName;
    static void*         creator() { return new RunLayerEditorTestsCmd(); }

    MStatus doIt(const MArgList&) override
    {
        // QWidget requires a QApplication. Maya standalone does not create one,
        // so we create it here if absent. QT_QPA_PLATFORM=offscreen (set in the
        // test env) ensures widget creation works without a display.
        static int           sArgc = 0;
        static QApplication* sApp  = nullptr;
        if (!QCoreApplication::instance()) {
            sApp = new QApplication(sArgc, nullptr);
        }

        // InitGoogleTest may only be called once per process — guard on first call.
        static bool sInitialized = false;
        if (!sInitialized) {
            int   argc    = 0;
            char* argv[1] = { nullptr };
            ::testing::InitGoogleTest(&argc, argv);
            // Remove default stdout printer so GTest output doesn't pollute Maya's output window.
            auto& listeners = ::testing::UnitTest::GetInstance()->listeners();
            delete listeners.Release(listeners.default_result_printer());
            sInitialized = true;
        }

        auto* collector = new JsonResultCollector();
        ::testing::UnitTest::GetInstance()->listeners().Append(collector);

        RUN_ALL_TESTS();

        // GTest owns listeners after Append(); Release() transfers ownership back.
        ::testing::UnitTest::GetInstance()->listeners().Release(collector);
        setResult(MString(collector->toJson().c_str()));
        delete collector;
        return MS::kSuccess;
    }
};

const MString RunLayerEditorTestsCmd::kName("mayaUsd_runLayerEditorTests");

} // namespace

// ── Plugin entry points ───────────────────────────────────────────────────────

MStatus initializePlugin(MObject obj)
{
    MFnPlugin plugin(obj, "Autodesk", "1.0", "Any");
    return plugin.registerCommand(
        RunLayerEditorTestsCmd::kName, RunLayerEditorTestsCmd::creator);
}

MStatus uninitializePlugin(MObject obj)
{
    MFnPlugin plugin(obj);
    return plugin.deregisterCommand(RunLayerEditorTestsCmd::kName);
}
