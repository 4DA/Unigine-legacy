/*	glass light shader
 *	defines: VERTEX, NV3X, TEYLOR
 *
 *			written by Alexander Zaprjagaev
 *			frustum@frustum.org
 *			http://frustum.org
 */

#ifdef VERTEX

<vertex>
!!ARBvp1.0

ATTRIB xyz = vertex.attrib[0];
PARAM mvp[4] = { state.matrix.mvp };

DP4 result.position.x, mvp[0], xyz;
DP4 result.position.y, mvp[1], xyz;
DP4 result.position.z, mvp[2], xyz;
DP4 result.position.w, mvp[3], xyz;

MOV result.color, 0;

END

<fragment>

!!ARBtec1.0

replace primary

END

#else

/*****************************************************************************/
/*                                                                           */
/* perpixel lighting                                                         */
/*                                                                           */
/*****************************************************************************/

<vertex_local0> icamera
<vertex_local1> ilight
<vertex_local2> light_color
<vertex_local3> parameter1

/*
 */
<vertex>

!!ARBvp1.0

ATTRIB xyz = vertex.attrib[0];
ATTRIB normal = vertex.attrib[1];
ATTRIB tangent = vertex.attrib[2];
ATTRIB binormal = vertex.attrib[3];
ATTRIB st = vertex.attrib[4];

PARAM mvp[4] = { state.matrix.mvp };
PARAM mv[4] = { state.matrix.modelview };
PARAM camera = program.local[0];
PARAM light = program.local[1];
PARAM light_color = program.local[2];
PARAM param = program.local[3];

DP4 result.position.x, mvp[0], xyz;
DP4 result.position.y, mvp[1], xyz;
DP4 result.position.z, mvp[2], xyz;
DP4 result.position.w, mvp[3], xyz;

TEMP dir;
SUB dir, light, xyz;
DP3 result.texcoord[1].x, tangent, dir;
DP3 result.texcoord[1].y, binormal, dir;
DP3 result.texcoord[1].z, normal, dir;
MOV result.texcoord[1].w, light.w;

SUB dir, camera, xyz;
DP3 result.texcoord[2].x, tangent, dir;
DP3 result.texcoord[2].y, binormal, dir;
DP3 result.texcoord[2].z, normal, dir;

MOV result.color, light_color;
MOV result.color.w, param.w;

END

/*
 */
<fragment>

#ifdef NV3X

/*****************************************************************************/
/*                                                                           */
/* NV3X fragment program code                                                */
/*                                                                           */
/*****************************************************************************/
!!FP1.0

DP3H H0.w, f[TEX1], f[TEX1];			// H0.w - |light - xyz|
MADH_SAT H5.w, H0.w, -f[TEX1].w, 1.0;	// H5.w - 1.0 - |light - xyz| / (radius * radius)

TEX H1, f[TEX1], TEX2, CUBE;			// H1 - light direction
MADX H1, H1, 2.0, -1.0;

TEX H2, f[TEX2], TEX2, CUBE;			// H2 - camera direction or half angle
MADX H2, H2, 2.0, -1.0;

#ifdef TEYLOR
	DP3X H6.y, H1, H1;
	DP3X H6.z, H2, H2;
	
	MADX H6, H6, -0.5, 0.5;
	
	MADX H1, H1, H6.y, H1;
	MADX H2, H2, H6.z, H2;
#endif	/* TEYLOR */

RFLH H1, { 0,0,1,0 }, H1;
DP3X_SAT H2.w, H2, H1;
POWH_SAT H2.w, H2.w, 16.0;

MULX H4, H2.w, H5.w;
MOVX H4.w, 1.0;
MULX o[COLH], H4, f[COL0];				// specular * attenuation * color

END

#else

/*****************************************************************************/
/*                                                                           */
/* ARB fragment program code                                                 */
/*                                                                           */
/*****************************************************************************/
!!ARBfp1.0

TEMP dist, light_dir, camera_dir, base, color, temp, reflect;

DP3 dist.w, fragment.texcoord[1], fragment.texcoord[1];
MAD_SAT dist.x, dist.w, -fragment.texcoord[1].w, 1.0;				// attenuation

TEX light_dir, fragment.texcoord[1], texture[2], CUBE;				// light direction
MAD light_dir, light_dir, 2.0, -1.0;

TEX camera_dir, fragment.texcoord[2], texture[2], CUBE;				// camera direction or half angle
MAD camera_dir, camera_dir, 2.0, -1.0;

#ifdef TEYLOR
	DP3 temp.y, light_dir, light_dir;
	DP3 temp.z, camera_dir, camera_dir;
	
	MAD temp, temp, -0.5, 0.5;
	
	MAD light_dir, light_dir, temp.y, light_dir;
	MAD camera_dir, camera_dir, temp.z, camera_dir;
#endif	/* TEYLOR */

DP3 temp.w, normal, light_dir;
MUL temp.w, temp.w, 2.0;
MAD reflect, normal, temp.w, -light_dir;
DP3_SAT color.w, reflect, camera_dir;
POW_SAT color.w, color.w, {0,0,0,16}.w;

MUL color, color.w, dist.w;
MOV color.w, 1.0;
MUL result.color, color, fragment.color;

END

#endif	/* NV3X */

#endif  /* VERTEX */
