/*	mirror ambient shader
 *
 *			written by Alexander Zaprjagaev
 *			frustum@frustum.org
 *			http://frustum.org
 */

<vertex_local0> parameter0

/*
 */
<vertex>

!!ARBvp1.0

ATTRIB xyz = vertex.attrib[0];

PARAM mvp[4] = { state.matrix.mvp };
PARAM texture[4] = { state.matrix.texture };
PARAM param = program.local[0];

DP4 result.position.x, mvp[0], xyz;
DP4 result.position.y, mvp[1], xyz;
DP4 result.position.z, mvp[2], xyz;
DP4 result.position.w, mvp[3], xyz;

DP4 result.texcoord[0].x, texture[0], xyz;
DP4 result.texcoord[0].y, texture[1], xyz;
DP4 result.texcoord[0].z, texture[2], xyz;
DP4 result.texcoord[0].w, texture[3], xyz;

MOV result.color, param;

END

/*
 */
<fragment>

!!ARBfp1.0

TEMP color;
TXP color, fragment.texcoord[0], texture[0], 2D;
MUL result.color, color, fragment.color;

END
