# Lock

By defining al_usdmaya_lock metadata of a prim, we want to lock transform (translate, rotate, scale) attributes of those Maya objects derived from the prim. Except for locking attributes, "pushToPrim" attribute on AL::usdmaya::nodes::Transform will also be disabled to ensure transform values not being changed by user manipulation in Maya. For data consistency, when a prim's lock state is flipped to "unlocked", transform attributes will be unlocked while "pushToPrim" attribute stays put.

## Metadata
al_usdmaya_lock is a token type metadata. It allows 3 options: "transform", "inherited" and "unlocked". When importing following USD example into Maya through AL_usdmaya_ProxyShapeImport command, translated AL_usdmaya_Transform node "geo" will have locked translate, rotate, scale attributes and "pushToPrim" attribute is turned off. This also applies to AL_usdmaya_Transform node "cam", since its lock state is set to "inherited"from its parent "geo". "accessory" explicitly sets al_usdmaya_lock to "none". Thus lock chain breaks here. If a prim doesn't set this metadata, by default its lock behaviour is "inherited".

```
#usda 1.0
(
    doc = """Generated from Composed Stage of root layer 
"""
)

def Xform "root"
{
    def Xform "geo" (
        al_usdmaya_lock = "transform"
    )
    {
        def Camera "cam" (
            al_usdmaya_lock = "inherited"
        )
        {
            def Xform "accessory" (
                al_usdmaya_lock = "unlocked"
            )
            {
            }
        }
    }
}
```

## ModelAPI

al_usdmaya_lock can be predefined in USD layer or changed via USD API in runtime. Meanwhile, AL_usd_ModelAPI::SetLock() and AL_usd_ModelAPI::GetLock() are provided as convenient interfaces to access this metadata. AL_usd_ModelAPI::ComputeLock() walks up the hierarchy and returns resolved lock state of a prim.