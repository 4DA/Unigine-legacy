/*	fog final shader
 *
 *			written by Alexander Zaprjagaev
 *			frustum@frustum.org
 *			http://frustum.org
 */

<vertex_local0>	viewport

/*
 */
<vertex>

!!ARBvp1.0

ATTRIB xyz = vertex.attrib[0];

PARAM mvp[4] = { state.matrix.mvp };
PARAM viewport = program.local[0];
PARAM fog_size = program.local[1];

DP4 result.position.x, mvp[0], xyz;
DP4 result.position.y, mvp[1], xyz;
DP4 result.position.z, mvp[2], xyz;
DP4 result.position.w, mvp[3], xyz;

RCP result.texcoord[0].x, viewport.z;
RCP result.texcoord[0].y, viewport.w;

END

/*
 */
<fragment>

!!ARBfp1.0

TEMP texcoord;
MUL texcoord, fragment.position, fragment.texcoord[0];
TEX result.color, texcoord, texture[0], 2D;

END
