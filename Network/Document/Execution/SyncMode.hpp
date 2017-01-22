#pragma once

namespace Network
{
enum class ExpressionPolicy {
  OnFirst,
  OnMajority,
  OnAll
};

enum class SyncMode {
  AsyncOrdered,
  SyncOrdered,
  AsyncUnordered,
  SyncUnordered
};


}
