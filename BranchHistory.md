# Build Branch

## Purpose

This build branch is a downstream branch from dev. The purpose is to merge in branches from open pull requests in the github maya-usd repo. This allows us to build these new features give a build to QA/Design for testing.

Note: you should never create a pull request to merge changes into this branch. Instead simply (selectively) merge in branches from github where an open PR exists.

The only file to manually modify in this branch would be this one.

Under no circumstances should you modify the existing history of the build branch. Force push and rebase are **not** allowed.

## Github remote repo

You need to have cloned the [internal maya-usd repo](https://git.autodesk.com/maya3d/maya-usd).

And then add the [public Github repo](https://github.com/Autodesk/maya-usd) as a remote.

In this case the internal repo named **origin** and the github is named **github**.

```
$ git remote -v
github  https://github.com/Autodesk/maya-usd (fetch)
github  https://github.com/Autodesk/maya-usd (push)
origin  https://git.autodesk.com/maya3d/maya-usd.git (fetch)
origin  https://git.autodesk.com/maya3d/maya-usd.git (push)
```

## How to update *build* branch from latest *dev*
```
git merge --no-ff github/dev
git push
- This push will automatically trigger the preflight build
```

## How to merge in a branch from open PR
```
git fetch --all
git merge --no-ff github/<branch_name>
git push
- This push will automatically trigger the preflight build
```
