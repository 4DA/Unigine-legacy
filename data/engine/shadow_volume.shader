/*	shadow volume shader
 *
 *			written by Alexander Zaprjagaev
 *			frustum@frustum.org
 *			http://frustum.org
 */

<vertex_local0>	ilight

<vertex>

!!ARBvp1.0

ATTRIB xyz = vertex.attrib[0];

PARAM mvp[4] = { state.matrix.mvp };
PARAM light = program.local[0];

TEMP v;
SUB v, xyz, light;
DP3 v.w, v, v;
RSQ v.w, v.w;
MUL v, v, v.w;
MUL v, v, 10000.0;

SUB v.w, 1.0, xyz.w;
MAD v, v, v.w, xyz;
MOV v.w, 1.0;

DP4 result.position.x, mvp[0], v;
DP4 result.position.y, mvp[1], v;
DP4 result.position.z, mvp[2], v;
DP4 result.position.w, mvp[3], v;

MOV result.color, { 0, 0.1, 0, 1.0 };

END

/* only for Radeons
 * analog of depth clamp extension
 */
#ifdef RADEON

<fragment>

!!ARBfp1.0

MOV_SAT result.depth, fragment.position.z;

END

#endif	/* DEPTH_CLAMP */
