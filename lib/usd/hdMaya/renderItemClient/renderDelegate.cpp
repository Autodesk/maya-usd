//-
// ==========================================================================
// Copyright 2021 Autodesk, Inc. All rights reserved.
//
// This computer source code and related instructions and comments are the
// unpublished confidential and proprietary information of Autodesk, Inc.
// and are protected under applicable copyright and trade secret law.
// They may not be disclosed to, copied or used by any third party without
// the prior written consent of Autodesk, Inc.
// ==========================================================================
//+

#include <hdMaya/renderItemClient/renderItemRenderDelegate.h>
#include <hdMaya/renderItemClient/material.h>

#include <pxr/imaging/hd/renderDelegate.h>
#include <pxr/imaging/hdSt/renderDelegate.h>

PXR_NAMESPACE_OPEN_SCOPE


/*! \brief  Constructor.
 */
HdMayaRenderItemRenderDelegate::HdMayaRenderItemRenderDelegate()
	: HdStRenderDelegate()
{
}

HdMayaRenderItemRenderDelegate::HdMayaRenderItemRenderDelegate(HdRenderSettingsMap const& settingsMap)
	: HdStRenderDelegate(settingsMap)
{
}

/*! \brief  Destructor.
 */
HdMayaRenderItemRenderDelegate::~HdMayaRenderItemRenderDelegate()
{
}

/*! \brief  Request to Allocate and Construct a new, VP2 specialized Rprim.

	\param typeId       the type identifier of the prim to allocate
	\param rprimId      a unique identifier for the prim
	\param instancerId  the unique identifier for the instancer that uses
						the prim (optional: May be empty).

	\return A pointer to the new prim or nullptr on error.
*/
HdSprim* HdMayaRenderItemRenderDelegate::CreateSprim(
	const TfToken& typeId,
	const SdfPath& primId)
{
	if (typeId == HdPrimTypeTokens->material)
	{
		return new HdMayaRenderItemMaterial(primId);
	}
	
	return HdStRenderDelegate::CreateSprim(typeId, primId);
}

/*! \brief  Destroy & deallocate Rprim instance
 */
void HdMayaRenderItemRenderDelegate::DestroySprim(HdSprim* rPrim) 
{ 
	HdStRenderDelegate::DestroySprim(rPrim);
}


PXR_NAMESPACE_CLOSE_SCOPE