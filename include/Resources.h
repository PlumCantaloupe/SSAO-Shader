#pragma once
#include "cinder/CinderResources.h"

//images
#define NOISE_SAMPLER	CINDER_RESOURCE( textures/, random.png, 99, IMAGE )

//shaders
#define SSAO_VERT			CINDER_RESOURCE( shaders/, SSAO_vert.glsl, 100, GLSL )
#define SSAO_FRAG			CINDER_RESOURCE( shaders/, SSAO_frag.glsl, 101, GLSL )
#define SSAO_FRAG_LIGHT		CINDER_RESOURCE( shaders/, SSAOL_frag.glsl, 102, GLSL )

#define NaDepth_VERT		CINDER_RESOURCE( shaders/, NormalDepthTexCreate_vert.glsl, 103, GLSL )
#define NaDepth_FRAG		CINDER_RESOURCE( shaders/, NormalDepthTexCreate_frag.glsl, 104, GLSL )

#define BBlender_VERT		CINDER_RESOURCE( shaders/, BasicBlender_vert.glsl, 105, GLSL )
#define BBlender_FRAG		CINDER_RESOURCE( shaders/, BasicBlender_frag.glsl, 106, GLSL )

#define BLUR_H_VERT			CINDER_RESOURCE( shaders/, Blur_h_vert.glsl, 107, GLSL )
#define BLUR_H_FRAG			CINDER_RESOURCE( shaders/, Blur_h_frag.glsl, 108, GLSL )

#define BLUR_V_VERT			CINDER_RESOURCE( shaders/, Blur_v_vert.glsl, 108, GLSL )
#define BLUR_V_FRAG			CINDER_RESOURCE( shaders/, Blur_v_frag.glsl, 109, GLSL )


