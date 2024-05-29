//
// Copyright Contributors to the MaterialX Project
// SPDX-License-Identifier: Apache-2.0
//

#include "MayaSourceCodeNode.h"

#include <MaterialXGenShader/HwShaderGenerator.h>

MATERIALX_NAMESPACE_BEGIN

ShaderNodeImplPtr MayaSourceCodeNode::create() { return std::make_shared<MayaSourceCodeNode>(); }

void MayaSourceCodeNode::initialize(const InterfaceElement& element, GenContext& context)
{
    // First let MaterialX look at the implementation
    SourceCodeNode::initialize(element, context);

    // See if a resolved _sourceFilename is actually a library file name:
    if (!_sourceFilename.isEmpty())
    {
        // Find the shortest relative sub path that the library resolver can find correctly, this is what emitLibraryInclude requires.
        size_t pathIndex = 0;
        const size_t maxPathIndex = _sourceFilename.size() - 1;

        FilePath relativePath = _sourceFilename[maxPathIndex - pathIndex];
        const FilePath libraryPrefix = context.getOptions().libraryPrefix;
        while (pathIndex < maxPathIndex) {
            FilePath fullFilename = libraryPrefix.isEmpty() ? relativePath : libraryPrefix / relativePath;
            if (context.resolveSourceFile(fullFilename, FilePath()) == _sourceFilename) {
                break;
            }
            ++pathIndex;
            relativePath = FilePath{_sourceFilename[maxPathIndex - pathIndex]} / relativePath;
        }

        if (pathIndex < maxPathIndex) {
            // Found a working relative path to a library source file:
            _librarySourceFileName = relativePath;
        }
    }
}

void MayaSourceCodeNode::emitFunctionDefinition(const ShaderNode& node, GenContext& context, ShaderStage& stage) const
{
    if (!_librarySourceFileName.isEmpty()) {
        DEFINE_SHADER_STAGE(stage, Stage::PIXEL)
        {
            // EmitLibraryInclude will make sure not to duplicate the code:
            const ShaderGenerator& shadergen = context.getShaderGenerator();
            shadergen.emitLibraryInclude(_librarySourceFileName, context, stage);
        }
        return;
    }

    // Delegate to MaterialX if it was not a simple library include case.
    SourceCodeNode::emitFunctionDefinition(node, context, stage);
}

MATERIALX_NAMESPACE_END
