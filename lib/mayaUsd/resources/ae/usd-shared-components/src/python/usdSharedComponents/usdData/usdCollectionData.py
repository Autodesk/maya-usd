from typing import AnyStr, Sequence
from ..data.collectionData import CollectionData
from .usdCollectionStringListData import CollectionStringListData
from .validator import validatePrim, validateCollection

from pxr import Sdf, Tf, Usd

class UsdCollectionData(CollectionData):
    def __init__(self, prim: Usd.Prim, collection: Usd.CollectionAPI):
        super().__init__()
        from ..common.host import Host
        self._includes = Host.instance().createStringListData(collection, True)
        self._excludes = Host.instance().createStringListData(collection, False)
        self._noticeKey = None
        self.setCollection(prim, collection)

    def __del__(self):
        # Note: base class does not have a __del__ so we don't call super() for it.
        self._untrackCollectionNotifications()

    def setCollection(self, prim: Usd.Prim, collection: Usd.CollectionAPI):
        '''
        Update which collecton on which prim is held by this data class.
        '''
        self._prim = prim
        self._collection = collection

        self._includes.setCollection(collection)
        self._excludes.setCollection(collection)

        self._untrackCollectionNotifications()
        self._trackCollectionNotifications()
        self._onObjectsChanged(None, None)

    def _trackCollectionNotifications(self):
        '''
        Register with the USD object changed notification, which gets
        converted to the class's dataChanged signal (and to each
        CollectionStringListData dataChanged signal)
        '''
        if self._prim:
            self._noticeKey = Tf.Notice.Register(
                Usd.Notice.ObjectsChanged, self._onObjectsChanged, self._prim.GetStage())

    def _untrackCollectionNotifications(self):
        '''
        Stop receiving USD notifications for the prim and collection.
        '''
        if self._noticeKey is None:
            return
        self._noticeKey.Revoke()
        self._noticeKey = None

    def _onObjectsChanged(self, notice, sender):
        # TODO: check if the collection was actually touched by this change!
        self.dataChanged.emit()
        self._includes.dataChanged.emit()
        self._excludes.dataChanged.emit()

    # USD Stage information
    
    @validatePrim(None)
    def getStage(self):
        '''
        Returns the USD stage in which the collection lives.
        '''
        return self._prim.GetStage()

    # Include and exclude

    @validateCollection(False)
    def includesAll(self) -> bool:
        '''
        Verify if the collection includes all by default.
        '''
        includeRootAttribute = self._collection.GetIncludeRootAttr()
        return bool(includeRootAttribute.Get())

    @validateCollection(False)
    def setIncludeAll(self, state: bool) -> bool:
        '''
        Sets if the collection should include all items by default.
        '''
        if self.includesAll() == state:
            return False
        includeRootAttribute = self._collection.GetIncludeRootAttr()
        includeRootAttribute.Set(state)
        return True
    
    def getIncludeData(self) -> CollectionStringListData:
        '''
        Returns the included items string list.
        '''
        return self._includes

    def getExcludeData(self) -> CollectionStringListData:
        '''
        Returns the excluded items string list.
        '''
        return self._excludes

    @validateCollection(False)
    def removeAllIncludeExclude(self) -> bool:
        '''
        Remove all included and excluded items.
        By design, we author a block collection opinion.
        '''
        self._collection.BlockCollection()
        return True

    # Expression

    @validateCollection('')
    def getExpansionRule(self):
        '''
        Returns expansion rule as a USD token.
        '''
        return self._collection.GetExpansionRuleAttr().Get()

    @validateCollection(False)
    def setExpansionRule(self, rule) -> bool:
        '''
        Sets the expansion rule as a USD token.
        '''
        if rule == self.getExpansionRule():
            return False
        self._collection.CreateExpansionRuleAttr(rule)
        return True

    @validateCollection('')
    def getMembershipExpression(self) -> AnyStr:
        '''
        Returns the membership expression as text.
        '''
        usdExpressionAttr = self._collection.GetMembershipExpressionAttr().Get()
        if usdExpressionAttr is None:
            return None
        return usdExpressionAttr.GetText()
    
    @validateCollection(False)
    def setMembershipExpression(self, textExpression: AnyStr) -> bool:
        '''
        Set the textual membership expression.
        '''
        from ..common.host import Host, MessageType

        usdExpression = ""
        usdExpressionAttr = self._collection.GetMembershipExpressionAttr().Get()
        if usdExpressionAttr != None:
            usdExpression = usdExpressionAttr.GetText()

        if usdExpression == textExpression:
            return False

        try:
            # Clear the attribute if the text is empty
            if not textExpression:
                self._collection.GetMembershipExpressionAttr().Clear()
                return True

            self._collection.CreateMembershipExpressionAttr(Sdf.PathExpression(textExpression))

            # If the include-all flag is set and there are not explicit include or exclude,
            # then turn off the include-all flag so that the expression works. Otherwise,
            # the include-all flag would make the expression useless since everything would
            # be included anyway.
            #
            # Note: do it after setting the expression, since the expression might be invalid.
            if self.includesAll() and not self._includes.getStrings() and not self._excludes.getStrings():
                includeRootAttribute = self._collection.GetIncludeRootAttr()
                includeRootAttribute.Set(False)
                Host.instance().reportMessage(
                    '"Include All" has been disabled for the expression to take effect.',
                    MessageType.INFO)

            return True
        except Exception:
            Host.instance().reportMessage(
                'Failed to update the collection {1} with the expression: {0}'.format(textExpression, self._collection.GetName()))
            return False
