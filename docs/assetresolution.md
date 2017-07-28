
# Versioning and Asset Resolution



## How we use AssetResolvers at Animal Logic
One requirement we have for the scenes we build in Maya is that they don't change over time i.e the versions of resolved USD asset references don't change - when I open a saved maya scene on successive days I want to see the same result. T

This is not a problem in the Pixar Pipeline or in others which use a source-control like asset management system where only the 'chosen' version of each asset is available on the filesystem (see discussions on USD Group about Pixar's "pinning" approach etc). 
In our files, we reference assets using a URI scheme which normally doesn't contain versioning information. In our Publishing system, all versions of all assets are available in parallel, new versions are pushed, and we use a packaging system to choose which version of each asset to use. 

The package information is passed to our AssetResolver system as configuration data. In order to ensure that when working with a maya scene it is not affected by changing versions, we choose to save this Resolver configuration as a string in our Maya Scene, and reapply it to our AssetResolver system before opening the USD Stage. 
Currently we rely on abusing a couple of methods in the USDResolverContext - GetAsDebugString() and ConfigureResolverForAsset (see [usd](https://github.com/PixarAnimationStudios/USD/blob/8858430becd5021e60aafcdeeb4f953a0a2a88d1/pxr/usdImaging/lib/usdviewq/mainWindow.py#L1074) but we are hoping to convince Pixar of the need for some new AR interface methods ([usdgooglegroup](https://groups.google.com/d/msg/usd-interest/N-ZKHwwlclU/4yzNVQsgAQAJ)). We're not entirely sure how general this kind of workflow is.
 
 
 ### AssetResolver Support - how we do it? Through separate on sceneload event 
 @todo fabricem could you add some more details here?
