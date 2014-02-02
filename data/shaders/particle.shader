/*	particle shader
 *
 *			written by Alexander Zaprjagaev
 *			frustum@frustum.org
 *			http://frustum.org
 */

<vertex_local0>	fog_color

<vertex>

!!ARBvp1.0

ATTRIB xyz = vertex.attrib[0];		// coordinate
ATTRIB attrib = vertex.attrib[1];	// texture coord + dx + dy
ATTRIB color = vertex.attrib[2];	// color
ATTRIB sincos = vertex.attrib[3];	// sin(rotation), cos(rotation)

PARAM mvp[4] = { state.matrix.mvp };
PARAM tmv[4] = { state.matrix.modelview.transpose };
PARAM fog_color = program.local[0];

TEMP v, dx, dy;

MOV v.z, 0.0;			// 2D

MOV v.x, attrib.z;		// dx
MOV v.y, 0.0;
DP3 dx.x, tmv[0], v;
DP3 dx.y, tmv[1], v;
DP3 dx.z, tmv[2], v;

MOV v.x, 0.0;
MOV v.y, attrib.w;		// dy
DP3 dy.x, tmv[0], v;
DP3 dy.y, tmv[1], v;
DP3 dy.z, tmv[2], v;

ADD v, dx, dy;
ADD v, v, xyz;

DP4 result.position.x, mvp[0], v;
DP4 result.position.y, mvp[1], v;
DP4 result.position.z, mvp[2], v;
DP4 result.position.w, mvp[3], v;

TEMP texcoord;

MOV v.z, 0.0;			// 2D

MOV v.x, sincos.y;
MOV v.y, -sincos.x;
DP3 texcoord.x, v, attrib;

MOV v.x, sincos.x;
MOV v.y, sincos.y;
DP3 texcoord.y, v, attrib;

MOV texcoord.z, 0.0;
MOV texcoord.w, 1.0;

ADD result.texcoord, texcoord, { 0.5, 0.5, 0.0, 0.0 };

MUL result.color, color, fog_color.w;

END

/*
 */
<fragment>

!!ARBtec1.0

modulate texture primary

END
