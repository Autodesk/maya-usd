# FAQ


## Which versions of Maya do you support?
We are building against 2017 and early (beta) versions of 2018 at the moment. This may change at any time We have tested with 2016, and there are some problems with Selection in VP2, see [build](./build.md) fore more info
 
## Which versions of USD do you support?
We try and stay in sync with new versions of USD released by Pixar, and build against new releases as soon as we can, but we can't offer any guarantees on how long this will take.
We're not currently supporting multiple API/ABI incompatible "variants" of USD. This may change once USD is on a more regular release cycle, and has hit V1.0.

## What guarantees do you provide about API/ABI Compatibility etc
At the moment - none - we reserve the right to break our API whenever we need to. 
As AL_USDMaya is primarily a plugin rather than a library (at the moment) this shouldn't be a huge problem.

## How often do we look at PRs and Issues? 
Ideally, ongoing - but we can't provide any guarantees about our response time

## How often do you do a release and/or push to the github repository
Ideally, quite often - but again, we can't provide any guarantees.

## Does it build for Windows or Mac?
At the moment the plugin builds on both Linux and Windows. As USD support for MacOS stabilises, others can contribute this functionality if they wish. We only use Linux internally at Animal Logic so that is the most tested path. Tests should also run on Windows
