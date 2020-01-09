/*
 * Copyright (C) 2012 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef __DVB_PARSER_H_
#define __DVB_PARSER_H_

#include <core/SkBitmap.h>
//#include <images/SkImageEncoder.h>

#include "DvbClut.h"
#include "DvbPage.h"
#include "DvbRegion.h"
#include "DvbObject.h"

#ifdef DVB_ENABLE_HD_SUBTITLE 
#include "DvbDds.h"
#endif



#include "DvbClutMgr.h"
#include "DVbPageMgr.h"
#include "DvbRegionMgr.h"
#include "DvbObjectMgr.h"

#ifdef DVB_ENABLE_HD_SUBTITLE 
#include "DVBDdsMgr.h"
#endif

namespace android
{

class DVBParser
{
    public:
        DVBParser();
        ~DVBParser();
        status_t prepareBitmapBuffer();
        status_t unmapBitmapBuffer();
		status_t parseSegment(UINT8* pui1_data,size_t size);
		INT32	 paintAllRegion();
		INT32	 paintPage();
		VOID	 ReleasePage();
        
		int mBitmapWidth;
        int mBitmapHeight;


        int mFd;
        void * mBitmapData;
		DvbPage*	mCurrPage;	
		SkBitmap*  bitmap;
#if	DVB_SUBTITLE_SAVE_AS_YUV_FILE
		status_t prepareYuvFile();
		status_t stopRecordYuvFile();
		VOID	 Record2YuvFile(SkBitmap&  bitmap,unsigned int width,unsigned int height);
#endif

private:
		DvbPageMgr		mPageMgr;
		
#ifdef DVB_ENABLE_HD_SUBTITLE 
		DvbDdsMgr		mDdsMgr;
#endif
		
		DvbRegionMgr	mRegionMgr;
		DvbObjectMgr	mObjectMgr;
		DvbClutMgr		mClutMgr;

		bool            mRecRefNewPage;


#if	DVB_SUBTITLE_SAVE_AS_YUV_FILE
		int mYuvFd;
#endif
};

}
#endif

