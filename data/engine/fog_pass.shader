/*	fog pass shader
 *
 *			written by Alexander Zaprjagaev
 *			frustum@frustum.org
 *			http://frustum.org
 */

<matrix0>			transform
<vertex_local0>		camera
<vertex_local1>		viewport
<fragment_local0>	fog_color

/*
 */
<vertex>

!!ARBvp1.0

PARAM mvp[4] = { state.matrix.mvp };
PARAM transform[4] = { state.matrix.program[0] };
PARAM camera = program.local[0];
PARAM viewport = program.local[1];

ATTRIB xyz = vertex.attrib[0];

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

MUL result.texcoord[0], dist, 2;

RCP result.texcoord[1].x, viewport.z;
RCP result.texcoord[1].y, viewport.w;

END

/*
 */
<fragment>

!!ARBfp1.0

PARAM fog_color = program.local[0];

TEMP front;
DP3 front.w, fragment.texcoord[0], fragment.texcoord[0];
RSQ front.w, front.w;
RCP front.w, front.w;

TEMP back;
MUL back, fragment.position, fragment.texcoord[1];
TEX back, back, texture[0], 2D;
DP3 back.w, back, { 1.0, 256.0, 0.0, 0.0 };

TEMP dist;
SUB dist.w, back.w, front.w;

MUL dist.w, dist.w, fog_color.w;

EX2 result.color.w, -dist.w;

MOV result.color.xyz, fog_color;

END
