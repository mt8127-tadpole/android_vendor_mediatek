/*
 * Copyright (C) 2008 The Android Open Source Project
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

#include <fcntl.h>
#include <errno.h>
#include <math.h>
#include <poll.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/select.h>

#include <cutils/log.h>

#include "BatchSensor.h"
#define LOG_TAG "BATCH"

#define SYSFS_PATH           "/sys/class/input"


/*****************************************************************************/
BatchSensor::BatchSensor()
    : SensorBase(NULL, "m_batch_input"),//BATCH_INPUTDEV_NAME
      mEnabled(0),
      mInputReader(128)//temp value for 128
{
    	mPendingEvent.version = sizeof(sensors_event_t);
    	mPendingEvent.sensor = 0;
    	mPendingEvent.type = SENSOR_TYPE_META_DATA;
    	memset(mPendingEvent.data, 0x00, sizeof(mPendingEvent.data));
		mEnabledTime =0;

		mPendingEvent.timestamp =0;
		mdata_fd = FindDataFd(); 	
    	if (mdata_fd >= 0) {
        strcpy(input_sysfs_path, "/sys/class/misc/m_batch_misc/");
        input_sysfs_path_len = strlen(input_sysfs_path);
    	}
	ALOGD("batch misc path =%s", input_sysfs_path);

}

BatchSensor::~BatchSensor() {
if (mdata_fd >= 0)
		close(mdata_fd);

}
int BatchSensor::FindDataFd() {
	int fd = -1;
	int num = -1;
	char buf[64]={0};
	char *devnum_dir = NULL;
	char buf_s[64] = {0};


	devnum_dir = "/sys/class/misc/m_batch_misc/batchdevnum";
	
	fd = open(devnum_dir, O_RDONLY);
	if (fd >= 0)
	{
		read(fd, buf, sizeof(buf));
		sscanf(buf, "%d\n", &num);
		close(fd);
	}else{
		return -1;
	}
	sprintf(buf_s, "/dev/input/event%d", num);
	fd = open(buf_s, O_RDONLY);
    ALOGE_IF(fd<0, "couldn't find input device");
	return fd;
}

void BatchSensor::GetSensorDiv(int div[])
{

}

int BatchSensor::enable(int32_t handle, int en)
{
    return -errno;
}
int BatchSensor::setDelay(int32_t handle, int64_t ns)
{
    return -errno;
}
int BatchSensor::batch(int handle, int flags, int64_t samplingPeriodNs, int64_t maxBatchReportLatencyNs)
{
	int res = 0;
	int fd = 0;
	if(maxBatchReportLatencyNs != 0)mEnabled = 1;
	else mEnabled = 0;
	if(flags & SENSORS_BATCH_DRY_RUN || flags & SENSORS_BATCH_WAKE_UPON_FIFO_FULL || (flags == 0)){
		
		strcpy(&input_sysfs_path[input_sysfs_path_len], "batchbatch");
		ALOGD("path:%s \r\n",input_sysfs_path);
		fd = open(input_sysfs_path, O_RDWR);
		if(fd<0)
		{
		  	ALOGD("no batch batch control attr\r\n" );
		  	return -1;
		}
		char buf[120] = {0};
		sprintf(buf, "%d,%d,%lld,%lld", handle, flags, samplingPeriodNs, maxBatchReportLatencyNs);
		ALOGD("batch value:%s ,size: %d \r\n",buf, strlen(buf)+1);
	       write(fd, buf, strlen(buf)+1);
		ALOGD("write path:%s \r\n",input_sysfs_path);
		close(fd);
		ALOGD("read path:%s \r\n",input_sysfs_path);
		fd = open(input_sysfs_path, O_RDWR);
		if(fd<0)
		{
		  	ALOGD("no batch batch control attr\r\n" );
		  	return -1;
		}
		char buf2[120] = {0};
	    	read(fd, buf2, sizeof(buf2));
		ALOGD("read value:%s  \r\n",buf2);
		sscanf(buf2, "%d", &res);
		ALOGD("return value:%d \r\n",res);	
	  	close(fd);		
	}else{
		ALOGD("batch mode is using invaild flag value for this operation!");
		res = -errno;
	}
    return res;
}

int BatchSensor::flush(int handle)
{
	int res = 0;
	int fd = 0;
	
	strcpy(&input_sysfs_path[input_sysfs_path_len], "batchflush");
	ALOGD("path:%s \r\n",input_sysfs_path);
	fd = open(input_sysfs_path, O_RDWR);
	if(fd<0)
	{
	  	ALOGD("no batch flush control attr\r\n" );
	  	return -1;
	}
	char buf[5] = {0};
	sprintf(buf, "%d", handle);
	ALOGD("flush value:%s ,size: %d \r\n",buf, strlen(buf)+1);
       write(fd, buf, strlen(buf)+1);
	ALOGD("flush value:%s after \r\n",buf);
	close(fd);
	ALOGD("read path:%s \r\n",input_sysfs_path);
	fd = open(input_sysfs_path, O_RDWR);
	if(fd<0)
	{
		ALOGD("no batch batch control attr\r\n" );
		return -1;
	}
	char buf2[5] = {0};
	read(fd, buf2, sizeof(buf2));
	ALOGD("read value:%s  \r\n",buf2);
	sscanf(buf2, "%d", &res);
	ALOGD("return value:%d \r\n",res);	
	close(fd);
    return 0;
}

int BatchSensor::readEvents(sensors_meta_data_event_t* data, int count)
{

    //ALOGE("fwq read Event 1\r\n");
    if (count < 1)
        return -EINVAL;

    ssize_t n = mInputReader.fill(mdata_fd);
    if (n < 0)
        return n;
    int numEventReceived = 0;
    input_event const* event;

    while (count && mInputReader.readEvent(&event)) {
        int type = event->type;
		ALOGE("yucong.... type\r\n");
        if (type == EV_ABS) 
		{
            	processEvent(event->code, event->value);
			//ALOGE("fwq2....\r\n");
        } 
	else if (type == EV_SYN) 
        {
            ALOGE("yucong....event sync\r\n");
            int64_t time = timevalToNano(event->time);
            mPendingEvent.timestamp = time;
            if (mEnabled) 
			{
                 	//ALOGE("fwq4....\r\n");
			     if (mPendingEvent.timestamp >= mEnabledTime) 
				 {
				    	//ALOGE("fwq5....\r\n");
				 	*data++ = mPendingEvent;
					numEventReceived++;
			     }
                 count--;
                
            }
        } 
		else if (type != EV_ABS) 
        { 
            ALOGE("BatchSensor: unknown event (type=%d, code=%d)",
                    type, event->code);
        }
        mInputReader.next();
    }
	//ALOGE("fwq read Event 2\r\n");
    return numEventReceived;
}

void BatchSensor::processEvent(int code, int value)
{
    //ALOGD("processEvent code=%d,value=%d\r\n",code, value);
    switch (code) {
	case EVENT_TYPE_SENSORTYPE:
		mPendingEvent.type = value;
		mPendingEvent.sensor= TypeToSensor(value);
		if(mPendingEvent.sensor<0)
			 ALOGE("BatchSensor: unknown sensor: %d, type:%d", mPendingEvent.sensor, value);
		break;
       case EVENT_TYPE_BATCH_X:
		mPendingEvent.acceleration.x = (float)value;
            	break;
       case EVENT_TYPE_BATCH_Y:
            	mPendingEvent.acceleration.y = (float)value;
            	break;
       case EVENT_TYPE_BATCH_Z:
            	mPendingEvent.acceleration.z = (float)value;
            	break;
	case EVENT_TYPE_BATCH_VALUE:
		mPendingEvent.data[0]= (float)value;
		break;
	case EVENT_TYPE_END_FLAG:
		mPendingEvent.type = SENSOR_TYPE_META_DATA;
		mPendingEvent.sensor = 0;
    }

	return;
}

int BatchSensor::TypeToSensor(int type)
{
	int sensor;
	switch(type){
		case SENSOR_TYPE_ACCELEROMETER:
			sensor = ID_ACCELEROMETER;
			break;
		case SENSOR_TYPE_MAGNETIC_FIELD:
			sensor = ID_MAGNETIC;
			break;
		case SENSOR_TYPE_ORIENTATION:
			sensor = ID_ORIENTATION;
			break;
		case SENSOR_TYPE_GYROSCOPE:
			sensor = ID_GYROSCOPE;
			break;
		case SENSOR_TYPE_LIGHT:
			sensor = ID_LIGHT;
			break;
		case SENSOR_TYPE_PROXIMITY:
			sensor = ID_PROXIMITY;
			break;
		case SENSOR_TYPE_PRESSURE:
			sensor = ID_PRESSURE;
			break;
		case SENSOR_TYPE_TEMPERATURE:
			sensor = ID_TEMPRERATURE;
			break;

		default:
			sensor = -1;
	}

	return sensor;
}

