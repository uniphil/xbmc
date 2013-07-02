#pragma once
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

#include "Interfaces/AESink.h"
#include <stdint.h>
#include "../Utils/AEDeviceInfo.h"

extern "C" {
#include <jack/types.h>
}
#include "jackblockingaudioio.hpp"

class CAESinkJACK : public IAESink
{
public:
  virtual const char *GetName() { return "Jack"; }

  CAESinkJACK();
  virtual ~CAESinkJACK();

  virtual bool Initialize  (AEAudioFormat &format, std::string &device);
  virtual void Deinitialize();
  virtual bool IsCompatible(const AEAudioFormat format, const std::string device);

  virtual double       GetDelay        ();
  virtual double       GetCacheTime    ();
  virtual double       GetCacheTotal   ();
  virtual unsigned int AddPackets      (uint8_t *data, unsigned int frames, bool hasAudio);
  virtual void         Drain           ();

  static void          EnumerateDevicesEx (AEDeviceInfoList &deviceInfoList, bool force = false);
private:

  JackCpp::BlockingAudioIO * jackBuffer;

  unsigned int outs;
  jack_nframes_t j_buffersize;
  float   m_msPerFrame;

  unsigned int m_buffered_frames;
  uint64_t m_last_update;

  void CacheUpdate();
};
