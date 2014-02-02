/*	glass ambient shader
 *
 *			written by Alexander Zaprjagaev
 *			frustum@frustum.org
 *			http://frustum.org
 */

<vertex_local0>	parameter0

<vertex>

!!ARBvp1.0

ATTRIB xyz = vertex.attrib[0];
ATTRIB st = vertex.attrib[4];

PARAM mvp[4] = { state.matrix.mvp };
PARAM param = program.local[0];

DP4 result.position.x, mvp[0], xyz;
DP4 result.position.y, mvp[1], xyz;
DP4 result.position.z, mvp[2], xyz;
DP4 result.position.w, mvp[3], xyz;

MOV result.texcoord, st;

MOV result.color, param;

END

<fragment>

!!ARBtec1.0

modulate texture primary

END
