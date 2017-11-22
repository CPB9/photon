#include "photon/ui/QModelBase.h"
#include "photon/model/Node.h"
#include "photon/model/NodeView.h"

namespace photon {

template class QModelItemDelegate<Node>;
template class QModelItemDelegate<NodeView>;
template class QModelBase<Node>;
template class QModelBase<NodeView>;
}
