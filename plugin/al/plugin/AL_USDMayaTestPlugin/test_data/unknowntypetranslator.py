from AL import usdmaya

from pxr import Tf


class UnknownTypeTranslator(usdmaya.TranslatorBase):

    def __init__(self):
        usdmaya.TranslatorBase.__init__(self)
    
    def initialize(self):
        return True
    
    # return the scheam type this class translates
    def getTranslatedType(self):
        return Tf.Type()

class KnownTypeTranslator(usdmaya.TranslatorBase):

    def __init__(self):
        usdmaya.TranslatorBase.__init__(self)
        
    def initialize(self):
        return True
    
    # return the scheam type this class translates
    def getTranslatedType(self):
        return Tf.Type.FindByName('AL_usd_ExamplePolyCubeNode')


ut = UnknownTypeTranslator()
usdmaya.TranslatorBase.registerTranslator(ut)

kt = KnownTypeTranslator()
usdmaya.TranslatorBase.registerTranslator(kt)
