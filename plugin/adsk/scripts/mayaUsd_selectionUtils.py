import ufe

def expandPathToSelection(path):
    """
    Retrieves the path of the selection item that contains the provided path.
    If there is no selection or no item in the selection is an ancestor of the item,
    then the original path itself is returned.
    """
    selection = ufe.GlobalSelection.get()
    ufePath = ufe.PathString.path(path)
    if selection.containsAncestor(ufePath):
        for sel in selection:
            subSel = ufe.Selection()
            subSel.append(sel)
            if subSel.containsAncestor(ufePath):
                return ufe.PathString.string(sel.path())
    return path
