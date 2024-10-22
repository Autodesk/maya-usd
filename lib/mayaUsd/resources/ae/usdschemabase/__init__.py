from .ae_template import AETemplate
from .enumCustomControl import EnumCustomControl
import ufe
if hasattr(ufe.Attributes, "getEnums"):
    AETemplate.prependControlCreator(EnumCustomControl.creator)
