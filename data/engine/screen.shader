/*	screen shader
 *
 *			written by Alexander Zaprjagaev
 *			frustum@frustum.org
 *			http://frustum.org
 */

<vertex_local0>	time

<vertex>

!!ARBvp1.0

ATTRIB xyz = vertex.position;
ATTRIB texcoord = vertex.texcoord;

PARAM mvp[4] = { state.matrix.mvp };
PARAM time = program.local[0];

DP4 result.position.x, mvp[0], xyz;
DP4 result.position.y, mvp[1], xyz;
DP4 result.position.z, mvp[2], xyz;
DP4 result.position.w, mvp[3], xyz;

MOV result.texcoord[0], texcoord;

TEMP offset;
MUL offset, time, { 27,13,0,0 };
MAD result.texcoord[1], texcoord, { 2,2,0,0 }, offset;

END

<fragment>

!!ARBtec1.0

replace texture

END

/*!!ARBfp1.0

TEMP color;
TEX color, fragment.texcoord[0], texture[0], 2D;

#TEMP noise;
#TEX noise, fragment.texcoord[1], texture[1], 2D;

#TEMP gray;
#DP3 gray, color, { 0.2126, 0.7152, 0.0722, 0.0 };

#MUL gray, gray, 0.5;
#MAD color, color, 0.5, gray;

#MAD result.color, noise, 0.2, color;

MOV result.color, color;

END
*/