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
