/*	light shader (diffuse, specular, gloss)
 *	defines: VERTEX, NV3X, HALF, TEYLOR
 *
 *			written by Alexander Zaprjagaev
 *			frustum@frustum.org
 *			http://frustum.org
 */

#ifdef VERTEX

/*****************************************************************************/
/*                                                                           */
/* simple vertex lighting                                                    */
/*                                                                           */
/*****************************************************************************/

<vertex_local0> ilight
<vertex_local1> light_color

<vertex>

!!ARBvp1.0

ATTRIB xyz = vertex.attrib[0];
ATTRIB normal = vertex.attrib[1];
ATTRIB st = vertex.attrib[4];

PARAM mvp[4] = { state.matrix.mvp };
PARAM light = program.local[0];
PARAM light_color = program.local[1];

DP4 result.position.x, mvp[0], xyz;
DP4 result.position.y, mvp[1], xyz;
DP4 result.position.z, mvp[2], xyz;
DP4 result.position.w, mvp[3], xyz;

MOV result.texcoord[0], st;

TEMP dir, color;

SUB dir, light, xyz;
DP3 dir.w, dir, dir;
MAD color.w, dir.w, -light.w, 1.0;
MAX color.w, color.w, 0.0;

RSQ dir.w, dir.w;
MUL dir, dir, dir.w;
DP3 color.xyz, dir, normal;

MUL color, color, color.w;

MUL result.color, color, light_color;

END

<fragment>

!!ARBtec1.0

modulate texture primary

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
PARAM camera = program.local[0];
PARAM light = program.local[1];
PARAM light_color = program.local[2];

DP4 result.position.x, mvp[0], xyz;
DP4 result.position.y, mvp[1], xyz;
DP4 result.position.z, mvp[2], xyz;
DP4 result.position.w, mvp[3], xyz;

MOV result.texcoord[0], st;

TEMP dir;
SUB dir, light, xyz;
DP3 result.texcoord[1].x, tangent, dir;
DP3 result.texcoord[1].y, binormal, dir;
DP3 result.texcoord[1].z, normal, dir;
MOV result.texcoord[1].w, light.w;

#ifdef HALF
	DP3 dir.w, dir, dir;
	RSQ dir.w, dir.w;
	MUL dir, dir, dir.w;
	
	TEMP half;
	SUB half, camera, xyz;
	DP3 half.w, half, half;
	RSQ half.w, half.w;
	MUL half, half, half.w;
	
	ADD half, half, dir;
	
	DP3 result.texcoord[2].x, tangent, half;
	DP3 result.texcoord[2].y, binormal, half;
	DP3 result.texcoord[2].z, normal, half;
#else
	SUB dir, camera, xyz;
	DP3 result.texcoord[2].x, tangent, dir;
	DP3 result.texcoord[2].y, binormal, dir;
	DP3 result.texcoord[2].z, normal, dir;
#endif /* HALF */

MOV result.color, light_color;

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

TEX H0, f[TEX0], TEX1, 2D;				// H0 - normal
MADX H0, H0, 2.0, -1.0;					// expand normal

TEX H1, f[TEX1], TEX2, CUBE;			// H1 - light direction
MADX H1, H1, 2.0, -1.0;

TEX H2, f[TEX2], TEX2, CUBE;			// H2 - camera direction or half angle
MADX H2, H2, 2.0, -1.0;

#ifdef TEYLOR
	DP3X H6.x, H0, H0;
	DP3X H6.y, H1, H1;
	DP3X H6.z, H2, H2;
	
	MADX H6, H6, -0.5, 0.5;
	
	MADX H0, H0, H6.x, H0;
	MADX H1, H1, H6.y, H1;
	MADX H2, H2, H6.z, H2;
#endif	/* TEYLOR */

DP3X H4.w, H0, H1;

TEX H3, f[TEX0], TEX0, 2D;				// H3 - diffuse
MULX H4, H4.w, H3;						// H4 - diffuse * base

#ifdef HALF
	DP3X_SAT H2.w, H0, H2;
	POWH_SAT H2.w, H2.w, 32.0;			// H2 - specular
#else
	RFLH H1, H0, H1;
	DP3X_SAT H2.w, H2, H1;
	POWH_SAT H2.w, H2.w, 16.0;
#endif	/* HALF */

MADX H4, H2.w, H3.w, H4;				// (diffuse * base + specular)
MULX H4, H4, H5.w;						// (diffuse * base + specular) * attenuation
MULX o[COLH], H4, f[COL0];				// (diffuse * base + specular) * attenuation * color

END

#else

/*****************************************************************************/
/*                                                                           */
/* ARB fragment program code                                                 */
/*                                                                           */
/*****************************************************************************/
!!ARBfp1.0

TEMP dist, light_dir, camera_dir, normal, base, color, temp, reflect;

DP3 dist.w, fragment.texcoord[1], fragment.texcoord[1];
MAD_SAT dist.x, dist.w, -fragment.texcoord[1].w, 1.0;				// attenuation

TEX normal, fragment.texcoord[0], texture[1], 2D;
MAD normal, normal, 2.0, -1.0;										// expand normal

TEX light_dir, fragment.texcoord[1], texture[2], CUBE;				// light direction
MAD light_dir, light_dir, 2.0, -1.0;

TEX camera_dir, fragment.texcoord[2], texture[2], CUBE;				// camera direction or half angle
MAD camera_dir, camera_dir, 2.0, -1.0;

#ifdef TEYLOR
	DP3 temp.x, normal, normal;
	DP3 temp.y, light_dir, light_dir;
	DP3 temp.z, camera_dir, camera_dir;
	
	MAD temp, temp, -0.5, 0.5;
	
	MAD normal, normal, temp.x, normal;
	MAD light_dir, light_dir, temp.y, light_dir;
	MAD camera_dir, camera_dir, temp.z, camera_dir;
#endif	/* TEYLOR */

DP3 color.w, normal, light_dir;

TEX base, fragment.texcoord[0], texture[0], 2D;
MUL color, color.w, base;

#ifdef HALF
	DP3_SAT color.w, normal, camera_dir;
	POW_SAT color.w, color.w, {0,0,0,32}.w;
#else
	DP3 temp.w, normal, light_dir;
	MUL temp.w, temp.w, 2.0;
	MAD reflect, normal, temp.w, -light_dir;
	DP3_SAT color.w, reflect, camera_dir;
	POW_SAT color.w, color.w, {0,0,0,16}.w;
#endif	/* HALF */

MAD color, color.w, base.w, color;
MUL color, color, dist.x;
MUL result.color, fragment.color, color;

END

#endif	/* NV3X */

#endif	/* VERTEX */
