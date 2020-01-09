/* Copyright Statement:
 *
 * This software/firmware and related documentation ("MediaTek Software") are
 * protected under relevant copyright laws. The information contained herein
 * is confidential and proprietary to MediaTek Inc. and/or its licensors.
 * Without the prior written permission of MediaTek inc. and/or its licensors,
 * any reproduction, modification, use or disclosure of MediaTek Software,
 * and information contained herein, in whole or in part, shall be strictly prohibited.
 */
/* MediaTek Inc. (C) 2010. All rights reserved.
 *
 * BY OPENING THIS FILE, RECEIVER HEREBY UNEQUIVOCALLY ACKNOWLEDGES AND AGREES
 * THAT THE SOFTWARE/FIRMWARE AND ITS DOCUMENTATIONS ("MEDIATEK SOFTWARE")
 * RECEIVED FROM MEDIATEK AND/OR ITS REPRESENTATIVES ARE PROVIDED TO RECEIVER ON
 * AN "AS-IS" BASIS ONLY. MEDIATEK EXPRESSLY DISCLAIMS ANY AND ALL WARRANTIES,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE OR NONINFRINGEMENT.
 * NEITHER DOES MEDIATEK PROVIDE ANY WARRANTY WHATSOEVER WITH RESPECT TO THE
 * SOFTWARE OF ANY THIRD PARTY WHICH MAY BE USED BY, INCORPORATED IN, OR
 * SUPPLIED WITH THE MEDIATEK SOFTWARE, AND RECEIVER AGREES TO LOOK ONLY TO SUCH
 * THIRD PARTY FOR ANY WARRANTY CLAIM RELATING THERETO. RECEIVER EXPRESSLY ACKNOWLEDGES
 * THAT IT IS RECEIVER'S SOLE RESPONSIBILITY TO OBTAIN FROM ANY THIRD PARTY ALL PROPER LICENSES
 * CONTAINED IN MEDIATEK SOFTWARE. MEDIATEK SHALL ALSO NOT BE RESPONSIBLE FOR ANY MEDIATEK
 * SOFTWARE RELEASES MADE TO RECEIVER'S SPECIFICATION OR TO CONFORM TO A PARTICULAR
 * STANDARD OR OPEN FORUM. RECEIVER'S SOLE AND EXCLUSIVE REMEDY AND MEDIATEK'S ENTIRE AND
 * CUMULATIVE LIABILITY WITH RESPECT TO THE MEDIATEK SOFTWARE RELEASED HEREUNDER WILL BE,
 * AT MEDIATEK'S OPTION, TO REVISE OR REPLACE THE MEDIATEK SOFTWARE AT ISSUE,
 * OR REFUND ANY SOFTWARE LICENSE FEES OR SERVICE CHARGE PAID BY RECEIVER TO
 * MEDIATEK FOR SUCH MEDIATEK SOFTWARE AT ISSUE.
 *
 * The following software/firmware and/or related documentation ("MediaTek Software")
 * have been modified by MediaTek Inc. All revisions are subject to any receiver's
 * applicable license agreements with MediaTek Inc.
 */

#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <pthread.h>
#include <string.h>
#include <sys/stat.h> 
#include <fcntl.h>
//#include <cutils/pmem.h>
#include <common.h>
#include <miniui.h>
#include <ftm.h>
#include <dlfcn.h>
#include "multimediaFactoryTest.h"
#include "MediaTypes.h"
#if defined(FEATURE_FTM_MEMINFO)

#define TAG "[FTM_MEMINFO] "
#define PAGE_SIZE 4096
enum { ITEM_PASS, ITEM_FAIL };

static item_t meminfo_items[] = 
{
    item(ITEM_PASS,   uistr_pass),
    item(ITEM_FAIL,   uistr_fail),
    item(-1, NULL),
};

//#define FTM_MEMINFO_DEBUG
#define TZ_MEMINFO_PATH "/sys/module/tz_module/parameters/memsz"
#define KL_MEMINFO_PATH "/sys/bus/platform/drivers/ddr_type/ddr_size"
#define DDR_TYPE_PATH   "/sys/bus/platform/drivers/ddr_type/ddr_type"

struct meminfo_info 
{
    int     mem_total;
#ifdef FTM_MEMINFO_DEBUG
    char      info[1024];
#else
    char 	  info[128];
#endif
    text_t    title;
    text_t    text;
    struct ftm_module *mod;
    struct itemview *iv;
};

#define mod_to_meminfo(p)     (struct meminfo_info*)((char*)(p) + sizeof(struct ftm_module))

unsigned int myatoui(char *buf)
{
    char *ptr = buf;
    unsigned int ret = 0;

    while(!((*ptr >= '0') && (*ptr <= '9')))ptr++;
        
    while((*ptr >= '0') && (*ptr <= '9'))
	{
		ret = ret*10 + (*ptr - '0');
		ptr++;
	}

    return ret;
}

static int get_meminfo(struct meminfo_info *meminfo)
{
	int ret;
	int fd = 0;
	char readbuf[32];
	unsigned int tz_mem_size, kl_mem_size, mem_total;
#ifdef FTM_MEMINFO_DEBUG
	char *ptr = meminfo->info;
#endif

	fd = open(TZ_MEMINFO_PATH, O_RDONLY);
	if(fd == -1)
	{
		LOGD(TAG"open %s fail!\n", TZ_MEMINFO_PATH);
		sprintf(meminfo->info,"open %s fail!\n", TZ_MEMINFO_PATH);
		return -1;
	}

	memset(readbuf, 0, sizeof(readbuf));
	ret = read(fd, readbuf, sizeof(readbuf));
	if(ret == -1)
	{
		LOGD(TAG"read %s fail!\n", TZ_MEMINFO_PATH);
		sprintf(meminfo->info,"read %s fail!\n", TZ_MEMINFO_PATH);
		return -1;
	}

    tz_mem_size = myatoui(readbuf);
#ifdef FTM_MEMINFO_DEBUG
	ptr += sprintf(ptr,"tz_memsz: %s, %d!\n", readbuf, tz_mem_size);
#endif
    close(fd);

	fd = open(KL_MEMINFO_PATH, O_RDONLY);
	if(fd == -1)
	{
		LOGD(TAG"open %s fail!\n", KL_MEMINFO_PATH);
		sprintf(meminfo->info,"open %s fail!\n", KL_MEMINFO_PATH);
		return -1;
	}

    memset(readbuf, 0, sizeof(readbuf));
	ret = read(fd, readbuf, sizeof(readbuf));
	if(ret == -1)
	{
		LOGD(TAG"read %s fail!\n", KL_MEMINFO_PATH);
		sprintf(meminfo->info,"read %s fail!\n", KL_MEMINFO_PATH);
		return -1;
	}
	
    kl_mem_size = myatoui(readbuf);
#ifdef FTM_MEMINFO_DEBUG
    ptr += sprintf(ptr,"kl_memsz: %s, %d!\n", readbuf, kl_mem_size);
#endif
    close(fd);


    fd = open(DDR_TYPE_PATH, O_RDONLY);
    if(fd == -1)
    {
        LOGD(TAG"open %s fail!\n", DDR_TYPE_PATH);
        sprintf(meminfo->info,"open %s fail!\n", DDR_TYPE_PATH);
        return -1;
    }

    memset(readbuf, 0, sizeof(readbuf));
    ret = read(fd, readbuf, sizeof(readbuf));
    if(ret == -1)
    {
        LOGD(TAG"read %s fail!\n", DDR_TYPE_PATH);
        sprintf(meminfo->info,"read %s fail!\n", DDR_TYPE_PATH);
        return -1;
    }
    
#ifdef FTM_MEMINFO_DEBUG
    ptr += sprintf(ptr,"DDR TYPE: %s \n", readbuf);
#endif
    close(fd);

    mem_total = tz_mem_size + kl_mem_size;
    mem_total = mem_total>>20;

    meminfo->mem_total = mem_total;

#ifdef FTM_MEMINFO_DEBUG
    sprintf(ptr,"[QW]Total memory size: %d MB\n", mem_total);
#else
    sprintf(meminfo->info, " Total memory size: %d MB\n DRAM TYPE: %s", mem_total, readbuf);
#endif
    return 0;
	
}
/*
 * meminfo_entry: factory mode entry function.
 * @param:
 * @priv:
 * Return error code.
 */
static int meminfo_entry(struct ftm_param *param, void *priv)
{
    char *ptr;
    int chosen;
	bool exit;
    int i;
    struct meminfo_info *meminfo = (struct meminfo_info *)priv;
    struct itemview *iv;
   
    LOGD(TAG "%s\n", __FUNCTION__);
	get_meminfo(meminfo);

    init_text(&meminfo->title, param->name, COLOR_YELLOW);
    init_text(&meminfo->text, &meminfo->info[0], COLOR_YELLOW);

    /* Create a itemview */
    if (!meminfo->iv) {
        iv = ui_new_itemview();
        if (!iv) {
            LOGD(TAG "No memory");
            return -1;
        }
        meminfo->iv = iv;
    }
    
    iv = meminfo->iv;
    iv->set_title(iv, &meminfo->title);
    iv->set_items(iv, meminfo_items, 0);
    iv->set_text(iv, &meminfo->text);      

meminfo_entry_exit:
    do {
        chosen = iv->run(iv, &exit);
        switch (chosen) {
        case ITEM_PASS:
        case ITEM_FAIL:
            /* report test results */
            if (chosen == ITEM_PASS) {
                meminfo->mod->test_result = FTM_TEST_PASS;
            } else if (chosen == ITEM_FAIL) {
                meminfo->mod->test_result = FTM_TEST_FAIL;
            }
			exit  = true;
            break;
        default:
            break;
        }

		iv->redraw(iv);
        if (exit) {
            break;
        }       
    } while (1);

	LOGD(TAG "meminfo entry exit\n");

    return 0;
}

/*
 * meminfo_init: factory mode initialization function.
 * Return error code.
 */

int meminfo_init(void)
{
    int index;
    int ret = 0;
    struct ftm_module *mod;
    struct meminfo_info *meminfo;
    //pid_t p_id;

    LOGD(TAG "%s\n", __FUNCTION__);

    /* Alloc memory and register the test module */
    mod = ftm_alloc(ITEM_MEMINFO, sizeof(struct meminfo_info));
    if (!mod)
        return -ENOMEM;

    meminfo = mod_to_meminfo(mod);
    meminfo->mod = mod;

    /* register the entry function to ftm_module */
    ret = ftm_register(mod, meminfo_entry, (void*)meminfo);

    return ret;
}

#endif  /* FEATURE_FTM_MEMINFO */

