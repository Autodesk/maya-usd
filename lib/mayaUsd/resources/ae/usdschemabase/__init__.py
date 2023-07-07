from .ae_template import AETemplate
from .custom_enum_control import customEnumControlCreator
import ufe
if hasattr(ufe.Attributes, "getEnums"):
    AETemplate.prependControlCreator(customEnumControlCreator)
