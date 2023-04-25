#pragma once

namespace magic {

struct IManaged {
  virtual ~IManaged() = default;

  virtual void AddRef() = 0;
  virtual void ReleaseRef() = 0;

};


}  // namespace magic