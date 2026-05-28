
# Find QT
if(QT_VERSION EQUAL "5")
	find_package(Qt5 REQUIRED COMPONENTS Core Gui Widgets)
elseif(QT_VERSION EQUAL "6")
	find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets)
else() 
	message(SEND_ERROR "Invalid QT Version : ${QT_VERSION}.")
endif()

find_package(Python3 COMPONENTS Interpreter Development)
find_package(USD REQUIRED)
find_package(UFE REQUIRED)
find_package(USDUFE REQUIRED)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# Add source to this project's executable.
add_library (UsdLayerEditor SHARED
	# Code
	abstractCommandHook.h
	abstractLayerEditorWindow.h
	batchSaveLayersUIDelegate.cpp
	batchSaveLayersUIDelegate.h
	customLayerData.cpp
	customLayerData.h
	dirtyLayersCountBadge.cpp
	dirtyLayersCountBadge.h
	generatedIconButton.cpp
	generatedIconButton.h
	layerEditorAPI.h
	layerEditorCommands.cpp
	LayerEditorCommands.h
	layerEditorWidget.cpp
	layerEditorWidget.h
	layerEditorWidgetManager.cpp
	layerEditorWidgetManager.h
	layerEditorWindow.h
	layerEditorWindow.cpp
	layerLocking.cpp
	layerLocking.h
	layerMuting.cpp
	layerMuting.h
	layers.cpp
	layers.h
	layerTreeItem.cpp
	layerTreeItem.h
	layerTreeItemDelegate.cpp
	layerTreeItemDelegate.h
	layerTreeModel.cpp
	layerTreeModel.h
	layerTreeView.cpp
	layerTreeView.h
	layerTreeViewStyle.h
	loadLayersDialog.cpp
	loadLayersDialog.h
	pathChecker.cpp
	pathChecker.h
	saveLayersDialog.cpp
	saveLayersDialog.h
	sessionState.cpp
	sessionState.h
	stageSelectorWidget.cpp
	stageSelectorWidget.h
	stringResources.cpp
	stringResources.h
	tokens.cpp
	tokens.h
	ufeCommandHook.h
	ufeCommandHook.cpp
	utilOptions.h
	utilString.h
	utilUI.cpp
	utilUI.h
	utilFileSystem.cpp
	utilFileSystem.h
	utilSerialization.cpp
	utilSerialization.h
	utilQT.cpp
	utilQT.h
	warningDialogs.cpp
	# Icon resources
	resources.qrc
)

target_include_directories( UsdLayerEditor PUBLIC
	${UFE_INCLUDE_DIR}
	${USDUFE_INCLUDE_DIR}
	${Python3_INCLUDE_DIRS}
)

# Link QT
if(QT_VERSION EQUAL 5)
	target_link_libraries(UsdLayerEditor Qt5::Core Qt5::Gui Qt5::Widgets) 
else()
	target_link_libraries(UsdLayerEditor Qt6::Core Qt6::Gui Qt6::Widgets)
endif()

# Link USD
target_link_libraries(UsdLayerEditor 
	usd
	sdf
	usdUtils
) 

# Link UFE/UsdUfe
target_link_libraries(UsdLayerEditor 
	${UFE_LIBRARY}
	${USDUFE_LIBRARY}
	${Python3_LIBRARIES}
)

set_property(TARGET UsdLayerEditor PROPERTY CXX_STANDARD 17)
set_property(TARGET UsdLayerEditor PROPERTY AUTOMOC ON)
set_property(TARGET UsdLayerEditor PROPERTY AUTORCC ON)
set_property(TARGET UsdLayerEditor PROPERTY AUTOUIC ON)

if (MSVC)
	set(MSVC_DEFINITIONS
		LAYEREDITOR_EXPORTS
		NOMINMAX
		_CRT_SECURE_NO_WARNINGS
		_SCL_SECURE_NO_WARNINGS
		TBB_SUPPRESS_DEPRECATED_MESSAGES
	)

	target_compile_definitions(UsdLayerEditor
		PUBLIC
		${MSVC_DEFINITIONS}
	)
	
	# Disable some warnings that arise alot from USD.
	add_definitions(/wd26495 /wd4305 /wd4244)
	
	# Parallel compile
 	add_definitions(/MP)
endif()

target_compile_definitions(UsdLayerEditor
	PUBLIC
	QT_NO_KEYWORDS
	BOOST_LIB_TOOLSET=${BOOST_LIB_TOOLSET}
)

install(TARGETS UsdLayerEditor
	DESTINATION lib)
install(FILES $<TARGET_PDB_FILE:UsdLayerEditor> DESTINATION lib)

file(GLOB HEADER_FILES "./*.h")
install(FILES ${HEADER_FILES} DESTINATION include/UsdLayerEditor)