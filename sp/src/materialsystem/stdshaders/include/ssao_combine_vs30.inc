// ALL SKIP STATEMENTS THAT AFFECT THIS SHADER!!!
// defined $LIGHTING_PREVIEW && defined $FASTPATH && $LIGHTING_PREVIEW && $FASTPATH

#ifndef SSAO_COMBINE_VS30_H
#define SSAO_COMBINE_VS30_H

#include "shaderapi/ishaderapi.h"
#include "shaderapi/ishadershadow.h"
#include "materialsystem/imaterialvar.h"

class ssao_combine_vs30_Static_Index
{
public:
	ssao_combine_vs30_Static_Index( IShaderShadow* pShaderShadow, IMaterialVar** params )
	{
	}

	int GetIndex() const
	{
		return 0;
	}
};

#define shaderStaticTest_ssao_combine_vs30 1


class ssao_combine_vs30_Dynamic_Index
{
public:
	ssao_combine_vs30_Dynamic_Index( IShaderDynamicAPI* pShaderAPI )
	{
	}

	int GetIndex() const
	{
		return 0;
	}
};

#define shaderDynamicTest_ssao_combine_vs30 1


#endif	// SSAO_COMBINE_VS30_H