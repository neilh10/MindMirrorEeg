/*
 * dim.h    -   ## Biomonitors Mind Mirror III ##
 *
 * Description
 *
 *
 *
 */
#ifndef dim_h
#define dim_h

/*
 * Constants
 */


#define eDimdType enum eDimType
 enum eDimType {_DimSnap_, /* Snap shot */
	 _DimStart_,/* Start of new buffer */
	 _DimInit_ /*Initialise Functions */
 };

/*
 * Module Prototypes
 */
enum inite_return dim_initialisation(Initt_system_state *ssp);
 void DimSnapRaw(eDimdType Type);
 tRtRawPkt *DimRtrvSnapRaw(void);


#endif /* dim_h */











