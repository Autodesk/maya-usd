
# Mesh Geometry: Overview

The following document details the way in which Maya mesh data is imported and exported from USD.  

# Mesh Geometry: Polygons & Vertices

When importing/exporting data from Maya, the mesh data is defined using an array of vertex positions, an array of vertex counts 
_(one entry per face, defining how many vertices are contained in that face)_, and an array of vertex indices _(a flattened list 
of all vertex indices)_. 

The following table describes the Maya and USD API calls that should be used to access mesh data.

|  Data        | Maya API      | UsdGeomMesh |
|--------------|---------------|-------------|
| Vertex Positions | MFnMesh::getRawPoints or MFnMesh::getPoints |
| Vertex Counts    | First argument of MFnMesh::getVertices | 
| Face Vertex Indices | Second argument of MFnMesh::getVertices | 


# Mesh Geometry: Polygon Holes

Within Maya, there are two methods of defining holes on mesh geometry, however only one method is supported for import/export 
via AL_USDMaya _(and the second method will likely cause errors in the exported geometry)_. 

Within Maya the *performPolyHoleFace* command will tag a face as invisible _(You can also access this command via the 
```Edit Mesh -> Assign Invisible Faces``` menu item)_. This information can be queried from USD via
```UsdGeomMesh::GetHoleIndicesAttr()```. The data stored is merely an array of indices that indicate which faces are tagged
as invisible.  

Maya also supports holes using the ```Mesh Tools -> Make Hole``` tool. Polygon holes created via this tool will not be exported 
by AL_USDMaya _(and infact, are liable to lead to errors within the exported geometry data)_. 


# Mesh Geometry: Vertex/Corner Creases

Vertex creases assigned to mesh geometry will be exported into a pair of arrays, one to store the vertex indices the crease 
has been applied to _(which maps into the Vertex Positions array)_, and an array of sharpness params that will match up with the 
crease indices. 

The following table describes the Maya and USD API calls that should be used to access the vertex crease information: 


|  Data        | Maya API      | UsdGeomMesh |
|--------------|---------------|-------------|
| Vertex Crease Indices | The vertexIds param of MFnMesh::getCreaseVertices | UsdGeomMesh::GetCornerIndicesAttr | 
| Vertex Crease Sharpness | The creaseData param of MFnMesh::getCreaseVertices | UsdGeomMesh::GetCornerSharpnessesAttr | 


# Mesh Geometry: Edge Creases

Edge creases on mesh geometry are defined using pairs of vertex indices that describe the edges that have crease weights assigned. 
This implies that the edge crease indices array will always be twice the length of the crease weights array. 

The following table describes the Maya and USD API calls that should be used to access the edge crease information: 


|  Data        | Maya API      | UsdGeomMesh | 
|--------------|---------------|-------------|
| Edge Crease Indices | The edgeIds param of MFnMesh::getCreaseEdges | UsdGeomMesh::GetCreaseIndicesAttr | 
| Edge Crease Sharpness | The creaseData param of MFnMesh::getCreaseEdges | UsdGeomMesh::GetCornerSharpnessesAttr | 
| Edge Crease Lengths | _unsupported_ | UsdGeomMesh::GetCreaseLengthsAttr  | 


# Mesh Geometry: Vertex Normals

Vertex Normals are exported using interpolation modes of either faceVarying, UsdGeomTokens->vertex, UsdGeomTokens->uniform, 
or UsdGeomTokens->constant.

|  Data        | Maya API      | UsdGeomMesh |
|--------------|---------------|-------------|
| Vertex Normals | MFnMesh::getRawNormals | UsdGeomPointBased::GetNormalsAttr | 


# Mesh Geometry: Prim Vars (UV coords)

UV coordinates stored within UsdGeomPrimvar objects, are exported as arrays of GfVec2f's with an optional set of indices. 
The AL_USDMaya export has the ability to specify a compaction mode that at the lowest level, will simply copy the data 
directly from Maya into USD _(i.e. an interpolation UsdGeomTokens->faceVarying, with indices)_. Additional modes exist 
that add varying degrees of testing of the UV sets to decide whether they can be described using constant, uniform, 
or vertex interpolation modes. It should be noted that higher compaction levels may result in smaller USD files on disk, 
but at the expense of an additional computational overhead at export time. 

The following tables indicate the supported combinations of interpolation mode and primvar indices :  

|  Interpolation Mode         | Indexed | Export | Import |
|-----------------------------|---------|--------|--------|
| UsdGeomTokens->constant     | No      | Yes    | Yes    |
| UsdGeomTokens->vertex       | No      | Yes    | Yes    |
| UsdGeomTokens->vertex       | Yes     | No     | Yes    |
| UsdGeomTokens->uniform      | No      | Yes    | Yes    |
| UsdGeomTokens->uniform      | Yes     | No     | Yes    |
| UsdGeomTokens->faceVarying  | No      | Yes    | Yes    |
| UsdGeomTokens->faceVarying  | Yes     | Yes    | Yes    |


A summary of the compaction level, and the processing performed are detailed in this table:


|  Compaction Mode           | Description                             |
|----------------------------|-----------------------------------------|
| None                       | No tests performed, UV coordinates will be exported as faceVarying with indices |
| Basic                      | If all UV coords are the same, the mode will be constant; If the UV indices match the vertex indices, the interpolation mode will be vertex; otherwise the mode will be faceVarying with indices. |
| Medium                     | If there is a single UV assignment per face, the interpolation mode will be uniform. Otherwise, this mode performs the same tests as Basic. |
| Extensive                  | Where as the Medium and Basic levels perform tests based on the indices, the Extensive mode tests the actual UV coordinate data directly. This is far more likely to find cases of uniform/vertex interpolation, and will remap the data accordingly. As a result of comparisons with the UV coordinate data, the worst case processing time at export is greater. |


The following table indicates the interpolation modes that may result from a specific compaction mode. The asterix indicates 
that a simplified index check is used rather than a full data check, so this may miss a number of cases in practice. It should 
be noted that from a simplicity point of view, that always using a compaction mode of 'none' implies that code further down the
pipeline only needs to worry about UV data in a single format. If however you wish to ensure the smallest assets possible for
disk / network reasons, then the Extensive mode will give you that option _(although you will have to implement more complex
processing at render time)_      
 

|  Interpolation Mode         | None    | Basic  | Medium | Extensive |
|-----------------------------|---------|--------|--------|-----------|
| UsdGeomTokens->constant     | No      | Yes    | Yes    | Yes       |
| UsdGeomTokens->vertex       | No      | Yes*   | Yes*   | Yes       |
| UsdGeomTokens->uniform      | No      | No     | Yes    | Yes       |
| UsdGeomTokens->faceVarying  | No      | No     | No     | Yes       |
| UsdGeomTokens->faceVarying with Indices  | Yes     | Yes    | Yes    | Yes       |


# Mesh Geometry: Prim Vars (colour sets)

If a UsdGeomPrimvar contains arrays of 4D or 3D floating point values, then they will be treated as a colour set. 
The behaviour when importing/exporting colour sets is mostly the same as for UV sets, however given that Maya does 
not provide any indices for colour sets, the behaviour differs slightly for export data.  


The following tables indicate the supported combinations of interpolation mode and primvar indices are supported for import and export:  

|  Interpolation Mode         | Indexed | Export | Import |
|-----------------------------|---------|--------|--------|
| UsdGeomTokens->constant     | No      | Yes    | Yes    |
| UsdGeomTokens->vertex       | No      | Yes    | Yes    |
| UsdGeomTokens->vertex       | Yes     | No     | Yes    |
| UsdGeomTokens->uniform      | No      | Yes    | Yes    |
| UsdGeomTokens->uniform      | Yes     | No     | Yes    |
| UsdGeomTokens->faceVarying  | No      | Yes    | Yes    |
| UsdGeomTokens->faceVarying  | Yes     | No     | Yes    |

This implies that only the following interpolation modes will be exported from AL_USDMaya at present. 
No attempt to generate indices will be made during export. 

|  Interpolation Mode         | None    | Basic  | Medium | Extensive |
|-----------------------------|---------|--------|--------|-----------|
| UsdGeomTokens->constant     | No      | Yes    | Yes    | Yes       |
| UsdGeomTokens->vertex       | No      | Yes    | Yes    | Yes       |
| UsdGeomTokens->uniform      | No      | No     | Yes    | Yes       |
| UsdGeomTokens->faceVarying  | Yes     | Yes    | Yes    | Yes       |


# Mesh Translation & Diffing

When using the AL_usdmaya_TranslatePrim command to make modifications to Mesh data within Maya; when 
converting the geometry from Maya back into USD, a diff process is run in order to determine which 
_(if any)_ of the geometry components _(e.g. vertices, normals, uv sets, etc)_ have changed. If a 
given component type has changed _(or has been added)_, then those geometry components will be 
written back to the prim into the layer set as the current edit target.  
 
This is designed to minimise the amount of data accumulated when making simple tweaks on existing 
geometry.   

































