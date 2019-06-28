
# Versioning and Asset Resolution



## How we use AssetResolvers at Animal Logic
One requirement we have for the scenes we build in Maya is that they don't change over time i.e the versions of resolved USD asset references don't change - when I open a saved maya scene on successive days I want to see the same result. 

This is not a problem in the Pixar Pipeline or in others which use a source-control like asset management system where only the 'chosen' version of each asset is available on the filesystem (see discussions on USD Group about Pixar's "pinning" approach etc). 
In our files, we reference assets using a URI scheme which normally doesn't contain versioning information. In our Publishing system, all versions of all assets are available in parallel, new versions are pushed, and we use a packaging system to choose which version of each asset to use. 

The package information is passed to our AssetResolver system as configuration data. In order to ensure that when working with a maya scene it is not affected by changing versions, we choose to save this Resolver configuration as a string in our Maya Scene, and reapply it to our AssetResolver system before opening the USD Stage. 

To enable this, when a stage is opened in AL_USDMaya, we will pass the filepath of the root USD file to the ConfigureResolverForAsset method of the AssetResolver. There is also a string attribute called "assetResolverConfig" on the proxyShape, it's contents (if non-empty) will be passed to  ConfigureResolverForAsset in preference to the filepath of the USD stage root.


