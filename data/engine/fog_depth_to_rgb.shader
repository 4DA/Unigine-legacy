/*	fog depth to rgb shader
 *
 *			written by Alexander Zaprjagaev
 *			frustum@frustum.org
 *			http://frustum.org
 */

<matrix0>		transform
<vertex_local0>	camera

/*
 */
<vertex>

!!ARBvp1.0

ATTRIB xyz = vertex.attrib[0];

PARAM mvp[4] = { state.matrix.mvp };
PARAM transform[4] = { state.matrix.program[0] };
PARAM camera = program.local[0];

DP4 result.position.x, mvp[0], xyz;
DP4 result.position.y, mvp[1], xyz;
DP4 result.position.z, mvp[2], xyz;
DP4 result.position.w, mvp[3], xyz;

TEMP dist;
DP4 dist.x, transform[0], xyz;
DP4 dist.y, transform[1], xyz;
DP4 dist.z, transform[2], xyz;
DP4 dist.w, transform[3], xyz;

SUB dist, camera, dist;

MUL result.texcoord[0], dist, 512;

END

/*
 */
<fragment>

!!ARBfp1.0

TEMP dist;
DP3 dist.w, fragment.texcoord[0], fragment.texcoord[0];
RSQ dist.w, dist.w;
RCP dist.w, dist.w;

MUL dist.y, dist.w, 0.003906250;
FLR dist.y, dist.y;

MAD dist.x, dist.y, -256.0, dist.w;

MUL result.color, dist, 0.003906250;

END
