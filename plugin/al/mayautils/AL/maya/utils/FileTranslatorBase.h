//
// Copyright 2017 Animal Logic
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
#pragma once
//----------------------------------------------------------------------------------------------------------------------
/// \file   FileTranslatorBase.h
/// \brief  This file contains a few macros and templates to help automate the tedious boiler plate
/// setup of
///         Maya import/export plugins.
/// \code
/// MAYA_TRANSLATOR_BEGIN(MyExporter, "My Exporter", true, true, "*.my", "*.my");
///
///   // specify the option names (These will uniquely identify the exporter options)
///   static const char* const kSomeBoolValue = "Some Bool Value";
///   static const char* const kSomeIntValue = "Some Int Value";
///   static const char* const kSomeFloatValue = "Some Float Value";
///   static const char* const kSomeStringValue = "Some String Value";
///
///   // provide a method to specify the import/export options
///   static MStatus specifyOptions(FileTranslatorOptions& options)
///   {
///     options.addFrame("My Exporter Options");
///     options.addBool(kSomeBoolValue, true);
///     options.addInt(kSomeIntValue, 42);
///     options.addFloat(kSomeFloatValue, 1.1111f);
///     options.addString(kSomeStringValue, "Cheesburgers");
///   }
///
///   // implement one or more of these:
///   MStatus reader(const MFileObject& file, const OptionsParser& options, FileAccessMode mode);
///   MStatus writer(const MFileObject& file, const OptionsParser& options, FileAccessMode mode);
///
/// MAYA_TRANSLATOR_END();
///
/// MStatus MyExporter::reader(const MFileObject& file, const OptionsParser& options, FileAccessMode
/// mode)
/// {
///   // query your options
///   bool someBoolValue = options.getBool(kSomeBoolValue);
///   int someIntValue = options.getInt(kSomeIntValue);
///   float someFloatValue = options.getFloat(kSomeFloatValue);
///   MString someStringValue = options.getString(kSomeStringValue);
///
///   // import your data
///
///   return MS::kSuccess; // done!
/// }
///
/// MStatus MyExporter::writer(const MFileObject& file, const OptionsParser& options, FileAccessMode
/// mode)
/// {
///   // query your options
///   bool someBoolValue = options.getBool(kSomeBoolValue);
///   int someIntValue = options.getInt(kSomeIntValue);
///   float someFloatValue = options.getFloat(kSomeFloatValue);
///   MString someStringValue = options.getString(kSomeStringValue);
///
///   // export your data
///
///   return MS::kSuccess; // done!
/// }
/// \endcode
///
/// When you come to register your plugin, just do the following:
///
/// \code
/// MStatus initializePlugin(MObject obj) {
///   MFnPlugin fn(obj);
///
///   return MyExporter::register(fn);
/// }
///
/// MStatus uninitializePlugin(MObject obj) {
///   MFnPlugin fn(obj);
///
///   return MyExporter::unregister(fn);
/// }
/// \endcode
//----------------------------------------------------------------------------------------------------------------------
#include "AL/maya/utils/FileTranslatorOptions.h"

#include <maya/MGlobal.h>
#include <maya/MPxFileTranslator.h>

namespace AL {
namespace maya {
namespace utils {

/// \brief  Macro to wrap some boiler plate creation of a file translator
#define AL_MAYA_TRANSLATOR_BEGIN(                                                   \
    ClassName, TranslatorName, HaveRead, HaveWrite, DefaultExtension, FilterString) \
    class ClassName : public AL::maya::utils::FileTranslatorBase<ClassName>         \
    {                                                                               \
    public:                                                                         \
        static constexpr const char* const kTranslatorName = TranslatorName;        \
        static constexpr const char* const kClassName = #ClassName;                 \
        static void*                       creator() { return new ClassName; }      \
                                                                                    \
    private:                                                                        \
        bool    haveReadMethod() const override { return HaveRead; }                \
        bool    haveWriteMethod() const override { return HaveWrite; }              \
        MString defaultExtension() const override { return DefaultExtension; }      \
        MString filter() const override { return FilterString; }                    \
                                                                                    \
    public:

/// \brief  Macro to wrap some boiler plate creation of a file translator
#define AL_MAYA_TRANSLATOR_END() \
    }                            \
    ;

//----------------------------------------------------------------------------------------------------------------------
/// \brief  A utility class that provides a 'unique' base class to derive new translator from.
//----------------------------------------------------------------------------------------------------------------------
template <typename T> class FileTranslatorBase : public MPxFileTranslator
{
public:
    /// \brief  unregister the file translator
    /// \param  plugin the MFnPlugin function set
    template <typename FnPlugin> static MStatus registerTranslator(FnPlugin& plugin)
    {
        if (MS::kSuccess == T::specifyOptions(m_options)) {
            m_options.generateScript(m_optionParser, m_defaultOptionString);

            MStatus status = plugin.registerFileTranslator(
                T::kTranslatorName, "", T::creator, T::kClassName, m_defaultOptionString.asChar());

            if (!status) {
                MGlobal::displayError(
                    MString("Failed to register translator: ") + T::kTranslatorName);
            }
            return status;
        }
        MGlobal::displayError(
            MString("Failed to generate options for translator: ") + T::kTranslatorName);
        return MS::kFailure;
    }

    /// \brief  unregister the file translator
    /// \param  plugin the MFnPlugin function set
    template <typename FnPlugin> static MStatus deregisterTranslator(FnPlugin& plugin)
    {
        if (MS::kSuccess == T::cleanupOptions(m_options)) {
            MStatus status = plugin.deregisterFileTranslator(T::kTranslatorName);
            if (!status) {
                MGlobal::displayError(
                    MString("Failed to deregister translator: ") + T::kTranslatorName);
            }
            return status;
        }
        MGlobal::displayError(
            MString("Failed to remove options for translator: ") + T::kTranslatorName);
        return MS::kFailure;
    }

    /// \brief  default fall back in case no options are needed in the derived translator
    /// \param  options the file translator options
    /// \return MS::kSuccess if options were correctly specified
    static MStatus specifyOptions(FileTranslatorOptions& options) { return MS::kSuccess; }

    /// \brief  default fall back in case no options are needed in the derived translator
    /// \param  options the file translator options
    /// \return MS::kSuccess if options were correctly cleaned up
    static MStatus cleanupOptions(FileTranslatorOptions& options) { return MS::kSuccess; }

    /// \brief  Override this method to read your files (do not use the version from
    /// MPxFileTranslator!) \param  file the file to read into maya \param  options a set of
    /// Key/Value pair options passed through from the MEL GUI \param  mode does this actually serve
    /// any purpose? \return a failure in this case (because you need to override to import the
    /// file)
    virtual MStatus
    reader(const MFileObject& file, const OptionsParser& options, FileAccessMode mode)
    {
        return MS::kFailure;
    }

    /// \brief  Override this method to write your files (do not use the version from
    /// MPxFileTranslator!) \param  file information about the file to export \param  options a set
    /// of Key/Value pair options passed through from the MEL GUI \param  mode are we exporting
    /// everything, or only the selected objects \return a failure in this case (because you need to
    /// override to import the file)
    virtual MStatus
    writer(const MFileObject& file, const OptionsParser& options, FileAccessMode mode)
    {
        return MS::kFailure;
    }

    /// \brief  access the registered translator options
    /// \return the options
    static FileTranslatorOptions& options() { return m_options; }

protected:
    static void setPluginOptionsContext(PluginTranslatorOptionsInstance* pluginOptions)
    {
        m_optionParser.setPluginOptionsContext(pluginOptions);
    }

private:
    MStatus
    reader(const MFileObject& file, const MString& optionsString, FileAccessMode mode) override
    {
        prepPluginOptions();
        MStatus status = m_optionParser.parse(optionsString);
        if (MS::kSuccess == status) {
            return reader(file, m_optionParser, mode);
        }
        MGlobal::displayError("Unable to parse the file translator options");
        return status;
    }

    MStatus
    writer(const MFileObject& file, const MString& optionsString, FileAccessMode mode) override
    {
        prepPluginOptions();
        MStatus status = m_optionParser.parse(optionsString);
        if (MS::kSuccess == status) {
            return writer(file, m_optionParser, mode);
        }
        MGlobal::displayError("Unable to parse the file translator options");
        return status;
    }

    virtual void prepPluginOptions() { }

    static MString               m_defaultOptionString;
    static OptionsParser         m_optionParser;
    static FileTranslatorOptions m_options;
};

template <typename T> MString               FileTranslatorBase<T>::m_defaultOptionString;
template <typename T> OptionsParser         FileTranslatorBase<T>::m_optionParser;
template <typename T> FileTranslatorOptions FileTranslatorBase<T>::m_options(T::kClassName);

//----------------------------------------------------------------------------------------------------------------------
} // namespace utils
} // namespace maya
} // namespace AL
//----------------------------------------------------------------------------------------------------------------------
