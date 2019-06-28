# Contributor License Agreement #
Before contributing code to AL_USDMaya, we ask that you sign a Contributor License Agreement (CLA).  At the root of the repo you can find the two possible CLAs:

+ [AL_USDMaya_Corporate_CLA.pdf](../AL_USDMaya_Corporate_CLA.pdf) please sign this one for corporate use
+ [AL_USDMaya_Individual_CLA.pdf](../AL_USDMaya_Individual_CLA.pdf) please sign this one if you're an individual contributor

Once your CLA is signed, send it to usdmaya@al.com.au (please make sure to include your github username) and wait for confirmation that we've received it.  After that, you can submit pull requests.


# Contribution Workflow
Before you begin working on a bug fix or new feature, please post an issue on GitHub to let everyone know about the feature you want to implement, or the bug that you found and intend to fix.
 
When you do begin your work, create a fork into your own private repository and work from there.

Your development should be based on `develop` (and not on `master`).

Here is an example (linux):

```shell
git clone https://github.com/your_fork/AL_USDMaya.git
cd AL_USD

# Add the original repository as a remote
git remote add al_origin https://github.com/AnimalLogic/AL_USDMaya.git

git fetch al_origin

git checkout -b my_feature al_origin/develop

```

Before submitting the pull request, make sure `my_feature` branch contains the relevant
commits only (rebasing and/or squashing your work if needed).

Also make sure to go through the checklist described in the pull request's template.

Note that once your PR has been approved, it might take some time before we merge it.
 

# Coding Conventions
Please follow the existing coding conventions and style in each file and in each library when adding new files.
