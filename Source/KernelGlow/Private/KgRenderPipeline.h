#pragma once

#include "CoreMinimal.h"

class FRDGBuilder;
class FRDGTexture;

namespace KgRender
{
	/** Task 3 placeholder：原地 tint（验证注入点，Task 4 移除）。视口参数需要时用公开的 FSceneView 传入。 */
	void RenderPlaceholderPass(FRDGBuilder& GraphBuilder, FRDGTextureRef SceneColor);
}
