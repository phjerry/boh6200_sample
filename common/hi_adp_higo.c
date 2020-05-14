
#include <stdio.h>
#include <dirent.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <math.h>

#include "hi_adp.h"
#include "hi_go.h"
#include "hi_go_decoder.h"

hi_handle hLayer = 0,hLayer_sd = 0, hLayerSurface = 0;

hi_s32 Higo_Initial(HIGO_LAYER_FLUSHTYPE_E type)
{
    HIGO_LAYER_INFO_S stLayerInfo;

    HIAPI_RUN_RETURN(HI_GO_Init());

    HIAPI_RUN_RETURN(HI_GO_GetLayerDefaultParam(HIGO_LAYER_HD_0, &stLayerInfo));
    SAMPLE_COMMON_PRINTF("HIGO_LAYER_HD_0 %d %d %d %d %d %d %d\n",
    	stLayerInfo.ScreenWidth,stLayerInfo.ScreenHeight,
    	stLayerInfo.CanvasWidth,stLayerInfo.CanvasHeight,
    	stLayerInfo.DisplayWidth,stLayerInfo.DisplayHeight,
    	stLayerInfo.LayerFlushType);

    stLayerInfo.LayerID = HIGO_LAYER_HD_0;
    stLayerInfo.LayerFlushType = type;

    HIAPI_RUN_RETURN(HI_GO_CreateLayer(&stLayerInfo, &hLayer));

    HIAPI_RUN_RETURN(HI_GO_GetLayerDefaultParam(HIGO_LAYER_SD_0, &stLayerInfo));
    SAMPLE_COMMON_PRINTF("HIGO_LAYER_HD_0 %d %d %d %d %d %d %d\n",
    	stLayerInfo.ScreenWidth,stLayerInfo.ScreenHeight,
    	stLayerInfo.CanvasWidth,stLayerInfo.CanvasHeight,
    	stLayerInfo.DisplayWidth,stLayerInfo.DisplayHeight,
    	stLayerInfo.LayerFlushType);

    stLayerInfo.LayerID = HIGO_LAYER_SD_0;
    HIAPI_RUN_RETURN(HI_GO_CreateLayer(&stLayerInfo, &hLayer_sd));

    HIAPI_RUN_RETURN(HI_GO_GetLayerSurface (hLayer, &hLayerSurface));

    HIAPI_RUN_RETURN(HI_GO_SetLayerSurface (hLayer_sd, hLayerSurface));

    HIAPI_RUN_RETURN(HI_GO_FillRect(hLayerSurface, NULL, 0xff0000ff, HIGO_COMPOPT_NONE));

    return HI_SUCCESS;
}

hi_void Higo_Deinitial()
{
    /**destroy layer*/
    if(hLayer)
    {
        HI_GO_DestroyLayer(hLayer);
    }

    if(hLayer_sd)
    {
        HI_GO_DestroyLayer(hLayer_sd);
    }
    /** deinitial */
    HI_GO_Deinit();

    hLayer = 0;
    hLayer_sd = 0;
    hLayerSurface = 0;
}


hi_s32 Higo_Dec(hi_char *pszFileName, hi_u32 index, hi_handle *pSurface,HIGO_DEC_PRIMARYINFO_S *pPrimaryInfo,HIGO_DEC_IMGINFO_S *pImgInfo)
{
    hi_s32 ret = HI_SUCCESS;
    hi_handle hDecoder;
    HIGO_DEC_ATTR_S stSrcDesc;

    if(HI_NULL == pszFileName)
    	return HI_FAILURE;

    /** create decoder */
    stSrcDesc.SrcType = HIGO_DEC_SRCTYPE_FILE;
    stSrcDesc.SrcInfo.pFileName = pszFileName;
    HIAPI_RUN_RETURN(HI_GO_CreateDecoder(&stSrcDesc, &hDecoder));

    if(pPrimaryInfo)
       HIAPI_RUN_RETURN(HI_GO_DecCommInfo(hDecoder, pPrimaryInfo));

    if(pImgInfo)
        HIAPI_RUN_RETURN(HI_GO_DecImgInfo(hDecoder, index, pImgInfo));

    if(pSurface)
        HIAPI_RUN(HI_GO_DecImgData(hDecoder, index, NULL, pSurface),ret);

    HIAPI_RUN(HI_GO_DestroyDecoder(hDecoder),ret);

    return ret;
}

hi_s32 Higo_DecWithAttr(hi_char *pszFileName, hi_u32 index, hi_handle *pSurface,HIGO_DEC_IMGATTR_S *pImgAttr)
{
    hi_s32 ret = HI_SUCCESS;
    hi_handle hDecoder;
    HIGO_DEC_ATTR_S stSrcDesc;

    if(HI_NULL == pszFileName|| HI_NULL == pSurface || HI_NULL == pImgAttr)
    	return HI_FAILURE;

    /** create decorder */
    stSrcDesc.SrcType = HIGO_DEC_SRCTYPE_FILE;
    stSrcDesc.SrcInfo.pFileName = pszFileName;
    HIAPI_RUN_RETURN(HI_GO_CreateDecoder(&stSrcDesc, &hDecoder));

    HIAPI_RUN(HI_GO_DecImgData(hDecoder, index, pImgAttr, pSurface),ret);

    HIAPI_RUN(HI_GO_DestroyDecoder(hDecoder),ret);

    return ret;
}
