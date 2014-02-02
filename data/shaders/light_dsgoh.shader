/*	light shader (diffuse, specular, gloss, offset, horizon)
 *	defines: VERTEX, NV3X, TEYLOR, OFFSET, HORIZON
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

SUB dir, camera, xyz;
DP3 result.texcoord[2].x, tangent, dir;
DP3 result.texcoord[2].y, binormal, dir;
DP3 result.texcoord[2].z, normal, dir;

MOV result.color, light_color;

END

/*
 */
<fragment>

#ifdef NV3X

/*	NV3X fragment program code
 */
!!FP1.0

DP3H H0.w, f[TEX1], f[TEX1];			// H0.w - |light - xyz|
MADH_SAT H5.w, H0.w, -f[TEX1].w, 1.0;	// H5.w - 1.0 - |light - xyz| / (radius * radius)

TEX H1, f[TEX1], TEX2, CUBE;
MADX H1, H1, 2.0, -1.0;

TEX H2, f[TEX2], TEX2, CUBE;
MADX H2, H2, 2.0, -1.0;

#ifdef OFFSET
	TEX R0, f[TEX0], TEX3, 2D;
	MADX R0, R0, 0.08, -0.04;
	MAD R0, R0, H2, f[TEX0];
	
	TEX H0, R0, TEX1, 2D;
	TEX H3, R0, TEX0, 2D;
#else
	TEX H0, f[TEX0], TEX1, 2D;			// H0 - normal
	TEX H3, f[TEX0], TEX0, 2D;			// H3 - diffuse
#endif	/* OFFSET */

MADX H0, H0, 2.0, -1.0;					// expand normal

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
MULX H4, H4.w, H3;						// H4 - diffuse * base

RFLH H1, H0, H1;
DP3X_SAT H2.w, H2, H1;
POWH_SAT H2.w, H2.w, 16.0;				// H2 - specular

MADX H4, H2.w, H3.w, H4;				// H4 - (diffuse * base + specular)
MULX H4, H4, H5.w;						// H4 - (diffuse * base + specular) * attenuation

#ifdef HORIZON
	TEX H0, f[TEX1], TEX4, CUBE;		// H0 - horizon lookup
#ifndef OFFSET
	MOV R0, f[TEX0];
#endif	/* OFFSET */
	MOVX R0.z, H0.x;
	TEX H1, R0, TEX5, 3D;				// H1 - horizon
	
	SUBX_SAT H0.w, H0.y, H1.x;			// self shadowing
	MULH_SAT H0, H0.w, 8.0;				// H0 - shadow
	
	MULX H4, H4, H0;					// H4 - (diffuse * base + specular) * attenuation * shadow
#endif	/* HORIZON */

MULX o[COLH], f[COL0], H4;

END

#else

/*	ARB fragment program code
 */
!!ARBfp1.0

PARAM specular_exponent = { 0.0, 0.0, 0.0, 16.0 };

TEMP dist, light_dir, camera_dir, normal, base, color, offset, temp, reflect;

DP3 dist.w, fragment.texcoord[1], fragment.texcoord[1];
MAD_SAT dist.x, dist.w, -fragment.texcoord[1].w, 1.0;				// attenuation

TEX light_dir, fragment.texcoord[1], texture[2], CUBE;
MAD light_dir, light_dir, 2.0, -1.0;								// light direction

TEX camera_dir, fragment.texcoord[2], texture[2], CUBE;
MAD camera_dir, camera_dir, 2.0, -1.0;								// camera direction

#ifdef OFFSET
	TEX offset, fragment.texcoord[0], texture[3], 2D;
	MAD offset, offset, 0.08, -0.04;
	MAD offset, offset, camera_dir, fragment.texcoord[0];
	
	TEX normal, offset, texture[1], 2D;
	TEX base, offset, texture[0], 2D;
#else
	TEX normal, fragment.texcoord[0], texture[1], 2D;
	TEX base, fragment.texcoord[0], texture[0], 2D;
#endif	/* OFFSET */

MAD normal, normal, 2.0, -1.0;										// expand normal

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
MUL color, color.w, base;

DP3 temp.w, normal, light_dir;
MUL temp.w, temp.w, 2.0;
MAD reflect, normal, temp.w, -light_dir;

DP3_SAT color.w, reflect, camera_dir;
POW_SAT color.w, color.w, specular_exponent.w;

MAD color, color.w, base.w, color;
MUL color, color, dist.x;

#ifdef HORIZON
	TEMP lookup, horizon, shadow;
	TEX lookup, fragment.texcoord[1], texture[4], CUBE;
#ifndef OFFSET
	TEMP offset;
	MOV offset, fragment.texcoord[0];
#endif	/* OFFSET */
	MOV offset.z, lookup.x;
	TEX horizon, offset, texture[5], 3D;
	
	SUB_SAT shadow.w, lookup.y, horizon.x;
	MUL_SAT shadow, shadow.w, 8.0;
	
	MUL color, color, shadow;
#endif	/* HORIZON */

MUL result.color, fragment.color, color;

END

#endif	/* NV3X */

#endif	/* VERTEX */
