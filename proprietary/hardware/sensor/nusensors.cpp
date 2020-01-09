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
#define LOG_TAG "Sensors"

#include <hardware/sensors.h>
#include <fcntl.h>
#include <errno.h>
#include <dirent.h>
#include <math.h>

#include <poll.h>
#include <pthread.h>

#include <linux/input.h>

#include <cutils/atomic.h>
#include <cutils/log.h>

#include "nusensors.h"
#include "Hwmsen.h"
#include "Acceleration.h"
#include "Magnetic.h"
#include "Proximity.h"
#include "Pressure.h"
#include "Temprature.h"
#include "Gyroscope.h"
#include "AmbienteLight.h"
#include "BatchSensor.h"

/*****************************************************************************/

struct sensors_poll_context_t {
	struct sensors_poll_device_1 device;// must be first
	
        sensors_poll_context_t();
        ~sensors_poll_context_t();
    int activate(int handle, int enabled);
    int setDelay(int handle, int64_t ns);
    int pollEvents(sensors_event_t* data, int count);
	int batch(int handle, int flags, int64_t samplingPeriodNs, int64_t maxBatchReportLatencyNs);
	int flush(int handle);

private:
    enum {
        hwmsen        = 0, 
	 	batchsensor,
        accel,
        magnetic,
        gyro,
        light,
        proximity,
        pressure,
        temperature,
        numSensorDrivers,
        numFds,
    };

	int handleToDriver(int handle) const {
	ALOGE("handleToDriver handle(%d)\n",handle);
        switch (handle) {
            case ID_ACCELEROMETER:
				 return accel;
            case ID_MAGNETIC:
            case ID_ORIENTATION:
				 return magnetic;
            case ID_PROXIMITY:
				 return proximity;
            case ID_LIGHT:
				 return light;
            case ID_GYROSCOPE:
				 return gyro;
		case ID_PRESSURE:
				 return pressure;
		case ID_TEMPRERATURE:
				 return temperature;
		default:
				break;
        }
        return -EINVAL;
    }

    static const size_t wake = numFds - 1;
    static const char WAKE_MESSAGE = 'W';
    struct pollfd mPollFds[numFds];
    int mWritePipeFd;
    SensorBase* mSensors[numSensorDrivers];
};

/*****************************************************************************/

sensors_poll_context_t::sensors_poll_context_t()
{
    mSensors[hwmsen] = new Hwmsen();
    mPollFds[hwmsen].fd = ((Hwmsen *)mSensors[hwmsen])->mdata_fd;
    mPollFds[hwmsen].events = POLLIN;
    mPollFds[hwmsen].revents = 0;

	mSensors[batchsensor] = new BatchSensor();
    mPollFds[batchsensor].fd = ((BatchSensor*)mSensors[batchsensor])->mdata_fd;
    mPollFds[batchsensor].events = POLLIN;
    mPollFds[batchsensor].revents = 0;

	mSensors[accel] = new AccelerationSensor();
    mPollFds[accel].fd = ((AccelerationSensor*)mSensors[accel])->mdata_fd;
    mPollFds[accel].events = POLLIN;
    mPollFds[accel].revents = 0;

	mSensors[magnetic] = new MagneticSensor();
    mPollFds[magnetic].fd = ((MagneticSensor*)mSensors[magnetic])->mdata_fd;
    mPollFds[magnetic].events = POLLIN;
    mPollFds[magnetic].revents = 0;

	mSensors[proximity] = new ProximitySensor();
    mPollFds[proximity].fd = ((ProximitySensor*)mSensors[proximity])->mdata_fd;
    mPollFds[proximity].events = POLLIN;
    mPollFds[proximity].revents = 0;

	mSensors[light] = new AmbiLightSensor();
    mPollFds[light].fd = ((AmbiLightSensor*)mSensors[light])->mdata_fd;
    mPollFds[light].events = POLLIN;
    mPollFds[light].revents = 0;	

	mSensors[gyro] = new GyroscopeSensor();
    mPollFds[gyro].fd = ((GyroscopeSensor*)mSensors[gyro])->mdata_fd;
    mPollFds[gyro].events = POLLIN;
    mPollFds[gyro].revents = 0;

	mSensors[pressure] = new PressureSensor();
    mPollFds[pressure].fd = ((PressureSensor*)mSensors[pressure])->mdata_fd;
    mPollFds[pressure].events = POLLIN;
    mPollFds[pressure].revents = 0;

	mSensors[temperature] = new TempratureSensor();
    mPollFds[temperature].fd = ((TempratureSensor*)mSensors[temperature])->mdata_fd;
    mPollFds[temperature].events = POLLIN;
    mPollFds[temperature].revents = 0;

    int wakeFds[2];
    int result = pipe(wakeFds);
    ALOGE_IF(result<0, "error creating wake pipe (%s)", strerror(errno));
    fcntl(wakeFds[0], F_SETFL, O_NONBLOCK);
    fcntl(wakeFds[1], F_SETFL, O_NONBLOCK);
    mWritePipeFd = wakeFds[1];

    mPollFds[wake].fd = wakeFds[0];
    mPollFds[wake].events = POLLIN;
    mPollFds[wake].revents = 0;
}

sensors_poll_context_t::~sensors_poll_context_t() {
    for (int i=0 ; i<numSensorDrivers ; i++) {
        delete mSensors[i];
    }
    close(mPollFds[wake].fd);
    close(mWritePipeFd);
}

int sensors_poll_context_t::activate(int handle, int enabled) 
{
	ALOGD( "activate handle =%d, enable = %d",handle, enabled );
	int err=0;
	
	int index = handleToDriver(handle);

	if(ID_ORIENTATION == handle)
	{
		((AccelerationSensor*)(mSensors[accel]))->enableNoHALDataAcc(enabled);
		((Hwmsen*)(mSensors[hwmsen]))->enableNoHALDataAcc(enabled);
	}
	
	if(NULL != mSensors[index] && index >0 )
	{
	   ALOGD( "use new sensor index=%d, mSensors[index](%x)", index, mSensors[index]);
	   err =  mSensors[index]->enable(handle, enabled);
	}
	
	if(err || index<0 )
	{
  		ALOGD("use old sensor err(%d),index(%d) go to old hwmsen\n",err,index);
		// notify to hwmsen sensor to support old architecture
		err =  mSensors[hwmsen]->enable(handle, enabled);
	}
	
    if (enabled && !err) {
        const char wakeMessage(WAKE_MESSAGE);
        int result = write(mWritePipeFd, &wakeMessage, 1);
        ALOGE_IF(result<0, "error sending wake message (%s)", strerror(errno));
    }
    return err;
}

int sensors_poll_context_t::setDelay(int handle, int64_t ns) 
{

    int err =0;
	
	
    int index = handleToDriver(handle);
	if(NULL != mSensors[index] && index >0)
	{
	   err = mSensors[index]->setDelay(handle, ns);
	   ALOGE("new setDelay handle(%d),ns(%lld)m, error(%d), index(%d)\n",handle,ns,err,index);
	}
	if(err || index<0)
	{
		ALOGE("new setDelay handle(%d),ns(%lld) err! go to hwmsen\n",handle,ns);
		// notify to hwmsen sensor to support old architecture
	 	err = mSensors[hwmsen]->setDelay(handle, ns);
	}
    return err;
}

int sensors_poll_context_t::batch(int handle, int flags, int64_t samplingPeriodNs, int64_t maxBatchReportLatencyNs)
{
	ALOGD( "batch handle =%d, flag=%d, sampling=%lld, batchtimeout=%lld",handle, flags, samplingPeriodNs, maxBatchReportLatencyNs);
	int err=0;
	
	int index = handleToDriver(handle);

	if(maxBatchReportLatencyNs == 0)
	{
		err = setDelay(handle, samplingPeriodNs);
		if(err != 0)
		{
			return err;
		}
		err = mSensors[index]->batch(handle, flags, samplingPeriodNs, maxBatchReportLatencyNs);//tell single sensors to disable there own polling and use
		if(err != 0)
		{
			return err;
		}
	}else{
		err = mSensors[index]->batch(handle, flags, samplingPeriodNs, maxBatchReportLatencyNs);//tell single sensors to disable there own polling and use
		if(err != 0)
		{
			return err;
		}
	}
	
	ALOGE("go to batchsensor\n", handle);
	// notify to batch sensor to support new architecture
	err = mSensors[batchsensor]->batch(handle, flags, samplingPeriodNs, maxBatchReportLatencyNs);
	
    return err;
}
int sensors_poll_context_t::flush(int handle)
{
	ALOGD( "flush handle =%d",handle);
	int err=0;
	return -1;
	int index = handleToDriver(handle);
 	err = mSensors[index]->flush(handle);
	ALOGE("go to batchsensor\n",handle);
	// notify to hwmsen sensor to support old architecture
 	err = mSensors[batchsensor]->flush(handle);
    return err;
}

int sensors_poll_context_t::pollEvents(sensors_event_t* data, int count)
{
    int nbEvents = 0;
    int n = 0;
	//ALOGE("pollEvents count =%d",count );

    do {
        // see if we have some leftover from the last poll()
        for (int i=0 ; count && i<numSensorDrivers ; i++) {
            SensorBase* const sensor(mSensors[i]);
            if ((mPollFds[i].revents & POLLIN) || (sensor->hasPendingEvents())) {
                int nb = sensor->readEvents(data, count);
                if (nb < count) {
                    // no more data for this sensor
                    mPollFds[i].revents = 0;
                }
                //if(nb < 0||nb > count)
                //	ALOGE("pollEvents count error nb:%d, count:%d, nbEvents:%d", nb, count, nbEvents);//for sensor NE debug
                count -= nb;
                nbEvents += nb;
                data += nb;
                //if(nb < 0||nb > count)
                //	ALOGE("pollEvents count error nb:%d, count:%d, nbEvents:%d", nb, count, nbEvents);//for sensor NE debug
            }
        }

        
        if (count) {
            // we still have some room, so try to see if we can get
            // some events immediately or just wait if we don't have
            // anything to return
            
            n = poll(mPollFds, numFds, nbEvents ? 0 : -1);
            if (n<0) {
                ALOGE("poll() failed (%s)", strerror(errno));
                return -errno;
            }
            if (mPollFds[wake].revents & POLLIN) {
                char msg;
                int result = read(mPollFds[wake].fd, &msg, 1);
                ALOGE_IF(result<0, "error reading from wake pipe (%s)", strerror(errno));
                ALOGE_IF(msg != WAKE_MESSAGE, "unknown message on wake queue (0x%02x)", int(msg));
                mPollFds[wake].revents = 0;
            }
        }

        // if we have events and space, go read them
    } while (n && count);

    return nbEvents;
}

/*****************************************************************************/

static int poll__close(struct hw_device_t *dev)
{
    sensors_poll_context_t *ctx = (sensors_poll_context_t *)dev;
    if (ctx) {
        delete ctx;
    }
    return 0;
}

static int poll__activate(struct sensors_poll_device_t *dev,
        int handle, int enabled) {
    sensors_poll_context_t *ctx = (sensors_poll_context_t *)dev;
    return ctx->activate(handle, enabled);
}

static int poll__setDelay(struct sensors_poll_device_t *dev,
        int handle, int64_t ns) {
    sensors_poll_context_t *ctx = (sensors_poll_context_t *)dev;
    return ctx->setDelay(handle, ns);
}

static int poll__poll(struct sensors_poll_device_t *dev,
        sensors_event_t* data, int count) {
    sensors_poll_context_t *ctx = (sensors_poll_context_t *)dev;
    return ctx->pollEvents(data, count);
}

static int poll__batch(struct sensors_poll_device_1 *dev,
        int handle, int flags, int64_t samplingPeriodNs, int64_t maxBatchReportLatencyNs) {
    sensors_poll_context_t *ctx = (sensors_poll_context_t *)dev;
    return ctx->batch(handle, flags, samplingPeriodNs, maxBatchReportLatencyNs);
}

static int poll__flush(struct sensors_poll_device_1 *dev,
        int handle) {
    sensors_poll_context_t *ctx = (sensors_poll_context_t *)dev;
    return ctx->flush(handle);
}

/*****************************************************************************/

int init_nusensors(hw_module_t const* module, hw_device_t** device)
{
    int status = -EINVAL;

    sensors_poll_context_t *dev = new sensors_poll_context_t();
    memset(&dev->device, 0, sizeof(sensors_poll_device_1));

    dev->device.common.tag = HARDWARE_DEVICE_TAG;
    dev->device.common.version  = 0;//SENSORS_DEVICE_API_VERSION_1_1;
    dev->device.common.module   = const_cast<hw_module_t*>(module);
    dev->device.common.close    = poll__close;
    dev->device.activate        = poll__activate;
    dev->device.setDelay        = poll__setDelay;
    dev->device.poll            = poll__poll;
    dev->device.batch           = poll__batch;
	dev->device.flush			= poll__flush;

    *device = &dev->device.common;
    status = 0;
    return status;
}
