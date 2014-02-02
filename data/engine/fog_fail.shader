/*	fog fail shader
 *
 *			written by Alexander Zaprjagaev
 *			frustum@frustum.org
 *			http://frustum.org
 */

<matrix0>		transform
<vertex_local0>	viewport
<vertex_local1>	fog_color

/*
 */
<vertex>

!!ARBvp1.0

ATTRIB xyz = vertex.attrib[0];

PARAM mvp[4] = { state.matrix.mvp };
PARAM transform[4] = { state.matrix.program[0] };
PARAM viewport = program.local[0];
PARAM fog_color = program.local[1];

DP4 result.position.x, mvp[0], xyz;
DP4 result.position.y, mvp[1], xyz;
DP4 result.position.z, mvp[2], xyz;
DP4 result.position.w, mvp[3], xyz;

RCP result.texcoord[0].x, viewport.z;
RCP result.texcoord[0].y, viewport.w;

MOV result.texcoord[1].x, fog_color.w;
MUL result.texcoord[1].y, fog_color.w, 256.0;
MOV result.texcoord[1].z, 0.0;

MOV result.color, fog_color;

END

/*
 */
<fragment>

!!ARBfp1.0

TEMP dist;
MUL dist, fragment.position, fragment.texcoord[0];
TEX dist, dist, texture[0], 2D;

DP3 dist.w, dist, fragment.texcoord[1];

EX2 result.color.w, -dist.w;

MOV result.color.xyz, fragment.color;

END
