#include "util.h"
#include "asset_manager.h"

TC_NAMESPACE_BEGIN

AssetManager &AssetManager::get_instance() {
  static AssetManager manager;
  return manager;
}

TC_NAMESPACE_END
