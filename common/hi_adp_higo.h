#ifndef     _COMMON_HIGO_H
#define     _COMMON_HIGO_H

extern hi_handle hLayer, hLayer_sd,hLayerSurface;

hi_s32 Higo_Initial(HIGO_LAYER_FLUSHTYPE_E type);
hi_void Higo_Deinitial();
hi_s32 Higo_Dec(hi_char *pszFileName, hi_u32 index, hi_handle *pSurface,
	HIGO_DEC_PRIMARYINFO_S *pPrimaryInfo,HIGO_DEC_IMGINFO_S *pImgInfo);
hi_s32 Higo_DecWithAttr(hi_char *pszFileName, hi_u32 index, hi_handle *pSurface,
	HIGO_DEC_IMGATTR_S *pImgAttr);

#endif
