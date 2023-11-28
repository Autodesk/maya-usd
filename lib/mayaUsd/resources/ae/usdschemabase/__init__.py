from .ae_template import AETemplate
import ufe
if hasattr(ufe.Attributes, "getEnums"):
    from .enum_custom_control import customEnumControlCreator
    AETemplate.prependControlCreator(customEnumControlCreator)
