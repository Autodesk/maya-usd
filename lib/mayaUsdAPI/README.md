# MayaUsdAPI: stable API library

This project is a binary stable library which contains a subset of the
UsdUfe & MayaUsd library calls. It is intended to be used by other Maya
plugins which require access to MayaUsd features in a binary stable way.

Linking with only the MayaUsdAPI library will ensure that your Maya
plugin will not be broken by any changes in UsdUfe or MayaUsd. Both
those libraries are at major version zero which means anything may
change at any time.
