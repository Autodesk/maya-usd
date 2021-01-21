__kernel void computeNormals(
    __global const float* positions,
    const unsigned int    vertexCount, // number of posisions and normals
                                       /*  Adjacency buffer is two distinct parts.
                                           First, two ints per vertex the offset and the valence. The valence is the number of adjacent
                                          vertices. The offset is the offset into the adjacency buffer to find the vertex ids of the
                                          adjacent vertices.                                    Next, a list of vertex ids of the
                                          adjacent vertices for each vertex, found                                    using the information from the first part of the
                                          buffer.
                                       */
    __global const int* adjacency,
    __global float*     normals)
{
    unsigned int vertexId = get_global_id(0);
    if (vertexId >= vertexCount)
        return;

    unsigned int  offsetIdx = vertexId * 2;
    int           offset = adjacency[offsetIdx];
    int           valence = adjacency[offsetIdx + 1];
    __global int* currAdj = &adjacency[offset];

    const float3 currVertex = vload3(vertexId, positions);
    float3       accumulatedNormal = (float3)(0.0f, 0.0f, 0.0f);

    for (int neighbour = 0; neighbour < valence; neighbour++) {
        float3 prevVertex = vload3(*currAdj++, positions);
        float3 nextVertex = vload3(*currAdj++, positions);
        accumulatedNormal += cross(nextVertex - currVertex, prevVertex - currVertex);
    }

    accumulatedNormal = normalize(accumulatedNormal);
    vstore3(accumulatedNormal, vertexId, normals);
}
