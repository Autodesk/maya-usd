# Lock

By defining al_usdmaya_lock metadata of a prim, we want to lock transform (translate, rotate, scale) attributes of those Maya objects derived from the prim. Except for locking attributes, "pushToPrim" attribute on AL::usdmaya::nodes::Transform will also be disabled to ensure transform values not being changed by user manipulation in Maya.

## Metadata
al_usdmaya_lock is a token type metadata. It allows two options: "transform" and "inherited". When importing following USD example into Maya through AL_usdmaya_ProxyShapeImport command, translated AL_usdmaya_Transform node "geo" will have locked translate, rotate, scale attributes and "pushToPrim" attributed is turned off. This also applies to AL_usdmaya_Transform node "cam", since its lock state is "inherited" from parent which is "geo". If lock of "geo" is released by setting its metadata al_usdmaya_lock to an empty token, "cam" will also be released.

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
        }
    }
}
```

## ModelAPI

al_usdmaya_lock can be predefined in USD layer or changed via USD API in runtime. Meanwhile, AL_usd_ModelAPI::SetLock() and AL_usd_ModelAPI::GetLock() are provided as convenient interfaces to manipulate this metadata.