import ufe

def expandPathToSelection(path):
    """
    Retrieves the path of the selection item that contains the provided path.
    If there is no selection or no item in the selection is an ancestor of the item,
    then the original path itself is returned.
    """
    selection = ufe.GlobalSelection.get()
    ufePath = ufe.PathString.path(path)
    # Note: containsAncestor does *not* include the path itself,
    #       so this will stop when we're at the top-most path
    #       that is still in the selection but its parent is not.
    while selection.containsAncestor(ufePath):
        ufePath = ufePath.pop()
    return ufe.PathString.string(ufePath)
