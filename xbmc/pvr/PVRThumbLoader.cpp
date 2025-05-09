/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "PVRThumbLoader.h"

#include "FileItem.h"
#include "FileItemList.h"
#include "ServiceBroker.h"
#include "TextureCache.h"
#include "imagefiles/ImageFileURL.h"
#include "pvr/PVRManager.h"
#include "utils/StringUtils.h"
#include "utils/log.h"

#include <chrono>

namespace PVR
{
bool CPVRThumbLoader::LoadItem(CFileItem* item)
{
  bool result = LoadItemCached(item);
  result |= LoadItemLookup(item);
  return result;
}

bool CPVRThumbLoader::LoadItemCached(CFileItem* item)
{
  return FillThumb(*item);
}

bool CPVRThumbLoader::LoadItemLookup(CFileItem* item)
{
  return false;
}

void CPVRThumbLoader::OnLoaderFinish()
{
  if (m_bInvalidated)
  {
    m_bInvalidated = false;
    CServiceBroker::GetPVRManager().PublishEvent(PVREvent::ChannelGroupsInvalidated);
  }
  CThumbLoader::OnLoaderFinish();
}

void CPVRThumbLoader::ClearCachedImage(CFileItem& item)
{
  const std::string thumb = item.GetArt("thumb");
  if (!thumb.empty())
  {
    CServiceBroker::GetTextureCache()->ClearCachedImage(thumb);
    if (m_textureDatabase->Open())
    {
      m_textureDatabase->ClearTextureForPath(item.GetPath(), "thumb");
      m_textureDatabase->Close();
    }
    item.SetArt("thumb", "");
    m_bInvalidated = true;
  }
}

void CPVRThumbLoader::ClearCachedImages(const CFileItemList& items)
{
  for (auto& item : items)
    ClearCachedImage(*item);
}

bool CPVRThumbLoader::FillThumb(CFileItem& item)
{
  // see whether we have a cached image for this item
  std::string thumb = GetCachedImage(item, "thumb");
  if (thumb.empty())
  {
    if (item.IsPVRChannelGroup())
      thumb = GetChannelGroupThumbURL(item);
    else
      CLog::LogF(LOGERROR, "Unsupported PVR item '{}'", item.GetPath());

    if (!thumb.empty())
    {
      SetCachedImage(item, "thumb", thumb);
      m_bInvalidated = true;
    }
  }

  if (thumb.empty())
    return false;

  item.SetArt("thumb", thumb);
  return true;
}

std::string CPVRThumbLoader::GetChannelGroupThumbURL(const CFileItem& channelGroupItem) const
{
  const auto now{std::chrono::system_clock::now()};
  return StringUtils::Format("{}?ts={}", // append timestamp to Thumb URL to enforce texture refresh
                             IMAGE_FILES::URLFromFile(channelGroupItem.GetPath(), "pvr"),
                             std::chrono::system_clock::to_time_t(now));
}

} // namespace PVR
