# Contributing to USD For Maya

## Contributor License Agreement #
Before contributing code to this project, we ask that you sign a Contributor License Agreement (CLA).

+ [AutodeskFormCorpContribAgmtForOpenSource.pdf](CLA/AutodeskFormCorpContribAgmtForOpenSource.pdf) please sign this one for corporate use
+ [AutodeskFormIndContribAgmtForOpenSource.pdf](CLA/AutodeskFormIndContribAgmtForOpenSource.pdf) please sign this one if you're an individual contributor

The documents include instructions on where to send the completed forms to.  Once a signed form has been received you will be able to submit pull requests.


## Filing Issues

### Suggestions

The USD for Maya project is meant to evolve with feedback - the project and its users greatly appreciate any thoughts on ways to improve the design or features. Please use the `enhancement` tag to specifically denote issues that are suggestions - this helps us triage and respond appropriately.

### Bugs

As with all pieces of software, you may end up running into bugs. Please submit bugs as regular issues on GitHub - Maya developers are regularly monitoring issues and will prioritize and schedule fixes.

The best bug reports include a detailed way to predictably reproduce the issue, and possibly even a working example that demonstrates the issue.

## Contributing Code

The USD for Maya project accepts and greatly appreciates contributions. The project follows the [fork & pull](https://help.github.com/articles/using-pull-requests/#fork--pull) model for accepting contributions.

It is highly recommended that an issue be logged on GitHub before any work is started.  This will allow for early feedback from other developers and avoid multiple parallel efforts.

### Pull Requests

When contributing code, please also include appropriate tests as part of the pull request, and follow the same comment and coding style as the rest of the project. Take a look through the existing code for examples of the testing and style practices the project follows. Also read and adhere to the the [Coding guidelines](doc/codingGuidelines.md), many of which are enforced by clang-format checks.

All development should happen against the [dev](https://github.com/Autodesk/maya-usd/tree/dev) branch of the repository. Please make sure the base branch of your pull request is set to the dev branch when submitting your pull request.

All pull requests require at least one approving code reviewer and successful preflight run. Running the preflight is a manual process which can be launched by [assigning the pull request to yourself](https://docs.github.com/en/issues/tracking-your-work-with-issues/assigning-issues-and-pull-requests-to-other-github-users#assigning-an-individual-issue-or-pull-request). A preflight consists of two actions: <i>clang_format_linter</i> and <i>build_preflight</i>. If the clang_format_linter steps fails then the build_preflight action is skipped. The build_preflight action will compile your changes and run the test suite on all the various platforms (Windows, Mac, Linux) and current Maya versions supported. To view the results of both clang_format_linter and build_preflight actions, in your pull request select the <i>Checks</i> tab and then select <i>Pre-flight build on pull request</i>. You'll see both jobs on the left and if either failed you can select it for more information about the failure. For a build failure it will list which jobs (if any) that failed. You can access the build logs by selecting <i>Summary</i> in the left pane and then in the right pane scroll down to the bottom to find a link for the <i>build logs</i> which will download a .zip file containing the build log for each of the builds.
