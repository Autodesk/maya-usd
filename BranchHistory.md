# Build Branch

## Purpose

This build branch is a downstream branch from dev. The purpose is to merge in branches from open pull requests in the github maya-usd repo. This allows us to build these new features and give a Maya build to QA/Design for testing.

Note: you should never create a pull request to merge changes into the build branch. Instead simply in dev branch and/or (selectively) merge in branches from github where an open PR exists.

The only file to manually modify in this branch would be this one.

Under no circumstances should you modify the existing history of the build branch. Force push and rebase are **not** allowed.

## Github remote repo

You need to have cloned the [internal maya-usd repo](https://git.autodesk.com/maya3d/maya-usd). The *master* and *dev* branches are automatically mirrored from public GitHub repo.

If you will be merging open PR's from GitHub or need to immediately merge dev (from GitHub), then add the [public Github repo](https://github.com/Autodesk/maya-usd) as a remote.

In this case the internal repo is named **origin** and the github repo is named **github**.

```
$ git remote -v
github  https://github.com/Autodesk/maya-usd (fetch)
github  https://github.com/Autodesk/maya-usd (push)
origin  https://git.autodesk.com/maya3d/maya-usd.git (fetch)
origin  https://git.autodesk.com/maya3d/maya-usd.git (push)
```

## How to update *build* branch from latest internal *dev* 
```
git fetch --all
git merge --no-ff origin/dev
git push
- This push will automatically trigger the preflight build
```

## How to update *build* branch from latest GitHub *dev*
```
git fetch --all
git merge --no-ff github/dev
git push
- This push will automatically trigger the preflight build
```

## How to merge in a branch from open PR on GitHub
Note: merging A PR from the internal repo is **not** allowed.
```
git fetch --all
git merge --no-ff github/<branch_name>
git push
- This push will automatically trigger the preflight build
```
