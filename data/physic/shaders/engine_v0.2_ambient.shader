/*	Engine_v0.2 shader
 *
 *			written by Alexander Zaprjagaev
 *			frustum@frustum.org
 *			http://frustum.org
 */

#ifdef VERTEX

<vertex_local0>	parameter0
<vertex_local1>	time

<vertex>

!!ARBvp1.0

ATTRIB xyz = vertex.attrib[0];
ATTRIB st = vertex.attrib[4];

PARAM mvp[4] = { state.matrix.mvp };
PARAM param = program.local[0];
PARAM time = program.local[1];
PARAM sin = program.local[2];
PARAM cos = program.local[3];

DP4 result.position.x, mvp[0], xyz;
DP4 result.position.y, mvp[1], xyz;
DP4 result.position.z, mvp[2], xyz;
DP4 result.position.w, mvp[3], xyz;

SUB result.texcoord[0].x, 1, st.x;
MAD result.texcoord[0].y, time.x, 0.05, st.y;

MOV result.color, param;

END

<fragment>

!!ARBtec1.0

modulate texture primary

END

#else

<vertex_local0>	parameter0
<vertex_local1>	time
<vertex_local2>	sin
<vertex_local3>	cos

<vertex>

!!ARBvp1.0

ATTRIB xyz = vertex.attrib[0];
ATTRIB st = vertex.attrib[4];

PARAM mvp[4] = { state.matrix.mvp };
PARAM param = program.local[0];
PARAM time = program.local[1];
PARAM sin = program.local[2];
PARAM cos = program.local[3];

DP4 result.position.x, mvp[0], xyz;
DP4 result.position.y, mvp[1], xyz;
DP4 result.position.z, mvp[2], xyz;
DP4 result.position.w, mvp[3], xyz;

SUB result.texcoord[0].x, 1, st.x;
MAD result.texcoord[0].y, time.x, 0.05, st.y;

TEMP temp;
MUL temp, st, { 1, 5, 0, 0 };

TEMP texcoord;

ADD texcoord.x, temp.x, sin.z;
ADD texcoord.y, temp.y, cos.z;
MAD texcoord.x, sin.w, 0.3, texcoord.x;
MAD texcoord.y, cos.w, 0.3, texcoord.y;

MUL result.texcoord[1], texcoord, 0.4;

ADD texcoord.x, temp.x, cos.z;
SUB texcoord.y, temp.y, sin.z;
MAD texcoord.x, sin.w, -0.3, texcoord.x;
MAD texcoord.y, cos.w, 0.3, texcoord.y;

MAD result.texcoord[2], texcoord, 0.4, 0.4;

MOV result.color, param;

END

<fragment>

!!ARBfp1.0

TEMP color, temp;
TEX color, fragment.texcoord[1], texture[1], 2D;
TEX temp, fragment.texcoord[2], texture[1], 2D;
ADD color, color, temp;

TEX temp, fragment.texcoord[0], texture[0], 2D;
MUL color, color, temp.x;

TEX color, color, texture[2], 2D;

MUL color, color, temp.y;
ADD color.z, color.z, temp.z;

MUL result.color, color, fragment.color;

END

#endif /* VERTEX */
