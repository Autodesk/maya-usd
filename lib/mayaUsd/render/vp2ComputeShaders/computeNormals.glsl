#version 430

layout( std140, binding=0 ) uniform Values
{
	uint VertexCount;
};

// This is a float3 but for buffer layout to be correct use float
layout( std430, binding=1 ) buffer Pos
{
	float Positions[ ];
};

layout( std430, binding=2 ) buffer Adj
{
	int Adjacency[ ];
};

layout( std430, binding=3 ) buffer RtoS
{
	int RenderingToScene[ ];
};

layout( std430, binding=4 ) buffer StoR
{
	int SceneToRendering[ ];
};

// This is a float3 but for buffer layout to be correct use float
layout( std430, binding=5 ) buffer Norm
{
	float Normals[ ];
};

layout( local_size_x = 256, local_size_y = 1, local_size_z = 1) in;

void main() {
	uint renderingVertexId = gl_GlobalInvocationID.x;
	uint renderingVertexOffset = renderingVertexId *3;

	if (renderingVertexId < VertexCount)
	{
		uint sceneVertexId = RenderingToScene[renderingVertexId];

		uint adjOffsetIdx = sceneVertexId*2;
		int offset = Adjacency[adjOffsetIdx];
		int valence = Adjacency[adjOffsetIdx+1];

		vec3 currVertex = vec3(Positions[renderingVertexOffset], Positions[renderingVertexOffset+1], Positions[renderingVertexOffset+2]);
		vec3 accumulatedNormal = vec3(0.0, 0.0, 0.0);

		for (int neighbour=0; neighbour<valence; neighbour++)
		{
			int prevVertexOffset = SceneToRendering[Adjacency[offset++]] * 3;
			vec3 prevVertex = vec3(Positions[prevVertexOffset], Positions[prevVertexOffset+1], Positions[prevVertexOffset+2]);
			int nextVertexOffset = SceneToRendering[Adjacency[offset++]] * 3;
			vec3 nextVertex = vec3(Positions[nextVertexOffset], Positions[nextVertexOffset+1], Positions[nextVertexOffset+2]);
			accumulatedNormal += cross(nextVertex - currVertex, prevVertex - currVertex);
		}

		accumulatedNormal = normalize(accumulatedNormal);

		Normals[renderingVertexOffset] = accumulatedNormal.x;
		Normals[renderingVertexOffset+1] = accumulatedNormal.y;
		Normals[renderingVertexOffset+2] = accumulatedNormal.z;
	}
}


