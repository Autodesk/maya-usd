//*****************************************************************************
// Copyright (c) 2024 Autodesk, Inc.
// All rights reserved.
//
// These coded instructions, statements, and computer programs contain
// unpublished proprietary information written by Autodesk, Inc. and are
// protected by Federal copyright law. They may not be disclosed to third
// parties or copied or duplicated in any form, in whole or in part, without
// the prior written consent of Autodesk, Inc.
//*****************************************************************************

#ifndef LOOKDEVX_USD_EXPORT_H_
#define LOOKDEVX_USD_EXPORT_H_

#if defined(_WIN32)
#if defined(LOOKDEVX_USD_SHARED)
#define LOOKDEVX_USD_EXPORT __declspec(dllexport)
#else
#define LOOKDEVX_USD_EXPORT __declspec(dllimport)
#endif
#elif defined(__GNUC__)
#define LOOKDEVX_USD_EXPORT __attribute__((visibility("default")))
#else
#error "Unsupported platform."
#endif

#endif
