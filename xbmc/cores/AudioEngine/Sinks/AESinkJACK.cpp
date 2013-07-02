/*
 *      Copyright (C) 2010-2012 Team XBMC
 *      http://www.xbmc.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include <iostream>
#include "system.h"

#include "AESinkJACK.h"
#include "jackblockingaudioio.hpp"

#include <stdint.h>
#include <limits.h>

#include "guilib/LocalizeStrings.h"
#include "dialogs/GUIDialogKaiToast.h"

#include "Utils/AEUtil.h"
#include "utils/StdString.h"
#include "utils/log.h"
#include "utils/MathUtils.h"
#include "utils/TimeUtils.h"
#include "settings/GUISettings.h"

#define CACHE_MAGIC 10000
#define DELAY_MAGIC 0.6

using std::cout;
using std::endl;

CAESinkJACK::CAESinkJACK()
{
  jackBuffer = 0;
  outs = 2;
}

CAESinkJACK::~CAESinkJACK()
{
}

bool CAESinkJACK::Initialize(AEAudioFormat &format, std::string &device)
{

  cout << "Starting JackCpp::BlockingAudioIO (" << device << "):" << endl;
  jackBuffer = new JackCpp::BlockingAudioIO("xbmc jack woo", outs, outs, CACHE_MAGIC, CACHE_MAGIC);
  jackBuffer->start();

  unsigned int j_samplerate = jackBuffer->getSampleRate();
  if (format.m_sampleRate != j_samplerate)
  {
    cout << "  -> fixing sample rate (to " << j_samplerate << ")..." << endl;
    format.m_sampleRate = j_samplerate;
  }

  if (format.m_dataFormat != AE_FMT_FLOAT)
  {
    cout << "  -> fixing format to double..." << endl;
    format.m_dataFormat = AE_FMT_FLOAT;
  }

  j_buffersize = jackBuffer->getBufferSize();
  cout << "  -> setting num frames (" << j_buffersize << ")..." << endl;
  format.m_frames = j_buffersize;

  if (format.m_frameSamples != outs)
  {
    cout << "  -> fixing samples to outs (" << outs << ")..." << endl;
    format.m_frameSamples = outs;
  }

  int j_framesize = outs * (CAEUtil::DataFormatToBits(format.m_dataFormat) >> 3);
  cout << "  -> setting framesize (" << j_framesize << ")..." << endl;
  format.m_frameSize = j_framesize;

  m_msPerFrame = 1000.0f / j_samplerate;
  m_buffered_frames = 0;
  m_last_update = CurrentHostCounter();

  cout << "  -> connecting to jack (" << outs << " channels)..." << endl ;
  for (unsigned int out = 0; out < outs; out++)
    jackBuffer->connectToPhysical(out, out);
  
  cout << "Jack initialized!" << endl;
  return true;
}

void CAESinkJACK::Deinitialize()
{
  if (! jackBuffer)
    return;

  for (unsigned int out = 0; out < outs; out++)
    jackBuffer->disconnectOutPort(out);
  jackBuffer->close();
  //delete jackBuffer; // segfaults?!?
  jackBuffer = 0;
}

bool CAESinkJACK::IsCompatible(const AEAudioFormat format, const std::string device)
{
  cout << "Checking compatibility (device: " << device << ")..." << endl;
  return false;
}

double CAESinkJACK::GetDelay()
{
  CacheUpdate();
  double jack_latency = j_buffersize * m_msPerFrame / 1000.0f;
  return GetCacheTime() + jack_latency + DELAY_MAGIC;
}

double CAESinkJACK::GetCacheTime()
{
  CacheUpdate(); 
  return (double)(m_buffered_frames * m_msPerFrame) / 1000.0f;
}

double CAESinkJACK::GetCacheTotal()
{
  return (double)(CACHE_MAGIC * m_msPerFrame) / 1000.0f;
}

unsigned int CAESinkJACK::AddPackets(uint8_t *data, unsigned int frames, bool hasAudio)
{
  CacheUpdate();
  m_buffered_frames += frames;

  float* jdata = (float*)data;
  for (unsigned int frame = 0; frame < frames; frame++)
    for (unsigned int out = 0; out < outs; out++)
      jackBuffer->write(out, jdata[out + frame * outs]);

  return frames;
}

void CAESinkJACK::Drain()
{
}

void CAESinkJACK::EnumerateDevicesEx (AEDeviceInfoList &deviceInfoList, bool force)
{
  CAEDeviceInfo deviceInfo;

  deviceInfo.m_deviceName = std::string("jack.sink.lalala");
  deviceInfo.m_displayName = "Jack Sink";
  deviceInfo.m_displayNameExtra = std::string("(lelele)");

  deviceInfoList.push_back(deviceInfo);
}

void CAESinkJACK::CacheUpdate()
{
  uint64_t now = CurrentHostCounter();
  unsigned int delta_time = (now - m_last_update) / 1000;
  unsigned int eaten_frames = delta_time / m_msPerFrame;
  m_buffered_frames -= (eaten_frames > m_buffered_frames ?
                        m_buffered_frames : eaten_frames );
  m_last_update = now;
}
