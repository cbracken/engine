// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "range.h"

#include <inttypes.h>

#include <algorithm>
#include <sstream>

namespace gfx {

std::string Range::ToString() const {
  std::ostringstream output;
  output << "{" << start() << "," << end() << "}" << std::endl;
  return output.str();
}

std::ostream& operator<<(std::ostream& os, const Range& range) {
  return os << range.ToString();
}

}  // namespace gfx
