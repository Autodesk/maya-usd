// //-
// // ==========================================================================
// // Copyright 2021 Autodesk, Inc. All rights reserved.
// //
// // This computer source code and related instructions and comments are the
// // unpublished confidential and proprietary information of Autodesk, Inc.
// // and are protected under applicable copyright and trade secret law.
// // They may not be disclosed to, copied or used by any third party without
// // the prior written consent of Autodesk, Inc.
// // ==========================================================================
// //+

// #ifndef MAYAHYDRALIB_RENDER_ITEM_RENDER_DELEGATE_H
// #define MAYAHYDRALIB_RENDER_ITEM_RENDER_DELEGATE_H

// #include <mayaHydraLib/adapters/adapter.h>
// #include <mayaHydraLib/adapters/adapterDebugCodes.h>
// #include <mayaHydraLib/utils.h>

// #include <pxr/imaging/hdSt/renderDelegate.h>

// #include <pxr/base/gf/matrix4d.h>
// #include <pxr/base/tf/token.h>
// #include <pxr/imaging/hd/meshTopology.h>
// #include <pxr/imaging/hd/renderIndex.h>
// #include <pxr/imaging/hd/sceneDelegate.h>
// #include <pxr/pxr.h>

// #include <maya/MMatrix.h>

// #include <functional>
// #include <unordered_map>
// #include <memory>

// PXR_NAMESPACE_OPEN_SCOPE

// using MayaHydraRenderItemBasisCurvesPtr = std::shared_ptr<class MayaHydraRenderItemBasisCurves>;

// class MayaHydraRenderItemRenderDelegate : public HdStRenderDelegate
// {
// public:
// 	MAYAHYDRALIB_API
// 		MayaHydraRenderItemRenderDelegate();
// 	MAYAHYDRALIB_API
// 		MayaHydraRenderItemRenderDelegate(HdRenderSettingsMap const& settingsMap);

// 	~MayaHydraRenderItemRenderDelegate() override;

// public:
// 	////////////////////////////////////////////////////////////////////////////
// 	///
// 	/// MayaHydraRenderItemRenderDelegate
// 	///
// 	////////////////////////////////////////////////////////////////////////////


// 	MAYAHYDRALIB_API
// 		HdSprim *CreateSprim(TfToken const& typeId,
// 			SdfPath const& sprimId) override;

// 	//MAYAHYDRALIB_API
// 	//	HdSprim *CreateFallbackSprim(TfToken const& typeId) override;

// 	MAYAHYDRALIB_API
// 		void DestroySprim(HdSprim *sPrim) override;

// };

// PXR_NAMESPACE_CLOSE_SCOPE

// #endif // MAYAHYDRALIB_RENDER_ITEM_ADAPTER_H