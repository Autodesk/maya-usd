from typing import AnyStr, Sequence
from ..data.collectionData import CollectionData
from .usdCollectionStringListData import CollectionStringListData
from ..common.host import Host

from pxr import Sdf, Tf, Usd

class UsdCollectionData(CollectionData):
    def __init__(self, prim: Usd.Prim, collection: Usd.CollectionAPI):
        super().__init__()
        self._includes = CollectionStringListData(collection, True)
        self._excludes = CollectionStringListData(collection, False)
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

    # Item picking

    def canPick(self) -> bool:
        '''
        Return if there is a UI to pick items.
        '''
        return False
    
    def pick(self, dialogTitle) -> Sequence[AnyStr]:
        '''
        Pick items and return the sequnce of picked items.
        '''
        # TODO: move host functionality in sub-class of this class, overriding pick.
        prims: Sequence[Usd.Prim] = Host.instance().pick(self._prim.GetStage(), dialogTitle=dialogTitle)
        if prims is None:
            return
        items = []
        for prim in prims:
            items.append(prim.GetPath())
        return items

    # Include and exclude

    def includesAll(self) -> bool:
        '''
        Verify if the collection includes all by default.
        '''
        includeRootAttribute = self._collection.GetIncludeRootAttr()
        return includeRootAttribute.Get()

    def setIncludeAll(self, state: bool):
        '''
        Sets if the collection should include all items by default.
        '''
        if self.includesAll() == state:
            return
        includeRootAttribute = self._collection.GetIncludeRootAttr()
        includeRootAttribute.Set(state)
    
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

    # Expression

    def getExpansionRule(self):
        '''
        Returns expansion rule as a USD token.
        '''
        return self._collection.GetExpansionRuleAttr().Get()

    def setExpansionRule(self, rule):
        '''
        Sets the expansion rule as a USD token.
        '''
        if rule == self.getExpansionRule():
            return
        self._collection.CreateExpansionRuleAttr(rule)

    def getMembershipExpression(self) -> AnyStr:
        '''
        Returns the membership expression as text.
        '''
        usdExpressionAttr = self._collection.GetMembershipExpressionAttr().Get()
        if usdExpressionAttr is None:
            return None
        return usdExpressionAttr.GetText()
    
    def setMembershipExpression(self, textExpression: AnyStr):
        '''
        Set the textual membership expression.
        '''
        usdExpression = ""
        usdExpressionAttr = self._collection.GetMembershipExpressionAttr().Get()
        if usdExpressionAttr != None:
            usdExpression = usdExpressionAttr.GetText()

        textExpression = self._expressionText.toPlainText()
        if usdExpression != textExpression:
            # assign default value if text is empty
            if textExpression == "":
                self._collection.CreateMembershipExpressionAttr()
            else:
                self._collection.CreateMembershipExpressionAttr(Sdf.PathExpression(textExpression))

            if self._expressionCallback != None:
                self._expressionCallback()

