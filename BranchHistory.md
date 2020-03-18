# Build Branch

## Purpose

This build branch is a downstream branch from dev. The purpose is to merge in branches from open pull requests in the github maya-usd repo. This allows us to build these new features give a build to QA/Design for testing.

Note: you should never create a pull request to merge changes into this branch. Instead simply (selectively) merge in branches from github where an open PR exists.

The only file to modify in this branch is this one.

## How it was initially created

```
git fetch --all --prune --prune-tags --tags
git checkout github/dev
git checkout -b build
```

## History of merges

The following is a list of the history of merges that were done in this build branch. Each time you modify the build branch you should add a section at the bottom outlining
the steps you did. 

```
git merge --no-ff github/implement_ufe_attribute_notify_dev
git merge --no-ff github/autodesk/MAYA-100225/fixes_al_unit_tests
git merge --no-ff github/donnels/MAYA-101325/new_import_ui_feature_wip
git push
```

```
git merge --no-ff github/autodesk/MAYA-100225/fixes_al_unit_tests
Note: this merged back in dev as Hamed merged dev to his branch.
git merge --no-ff github/ufe_bbox_support_dev
git add BranchHistory.md
git commit -m "Adding Branch History file"
git push
```

```
Jan 28, 2020:
git merge --no-ff github/donnels/MAYA-101325/new_import_ui_feature_wip
git merge --no-ff github/autodesk/MAYA-100225/fixes_al_unit_tests
git merge --no-ff github/dev
git merge --no-ff github/adsk/MAYA-102918/add_mayausd_export_translator
git merge --no-ff github/ufe_bbox_support_dev
git push
```

```
Jan 30, 2020:
git merge --no-ff github/dev
git push
```

```
Feb 05, 2020:
git merge --no-ff github/donnels/MAYA-101325/new_import_ui_feature_wip
git merge --no-ff github/dev
git push
```

```
Feb 06, 2020:
git merge --no-ff github/dev
git merge --no-ff github/chenh/MAYA-101845/usd-geom-curves
git push
```

```
Feb 11, 2020:
git merge --no-ff github/dev
git merge --no-ff github/donnels/MAYA-102920/rpath_changes_for_maya_build
git merge --no-ff github/donnels/MAYA-103070/protect_importui_from_missing_command
git push
```

```
Feb 18, 2020:
git merge --no-ff github/adsk/MAYA-102918/add_mayausd_export_translator
git merge --no-ff github/donnels/MAYA-102920/rpath_changes_for_maya_build
git merge --no-ff github/dev
git push
```

```
Feb 25, 2020:
git merge --no-ff github/adsk/MAYA-102918/add_mayausd_export_translator
git merge --no-ff github/donnels/MAYA-103070/minimal_import_ui_pr113
git merge --no-ff github/Autodesk/tremblp/MAYA-103495/contextual_operations
git merge --no-ff github/roussel/MAYA-103409/Minimal_Create_USD_Stage
git merge --no-ff github/dev
```

```
Feb 27, 2020:
git merge --no-ff github/adsk/MAYA-102918/add_mayausd_export_translator
git merge --no-ff github/fowlert/MAYA-103561/enable_usdz_read
git merge --no-ff github/roussel/MAYA-103409/Minimal_Create_USD_Stage
```

```
Mar 04, 2020:
git merge --no-ff github/dev
```

```
Mar 05, 2020:
git merge --no-ff github/dev
```

```
Mar 17, 2020:
git merge --no-ff github/dev
git merge --no-ff github/donnels/MAYA-103655/implement_object3d_visibility
```

```
Mar 18, 2020:
git merge --no-ff github/dev
```
