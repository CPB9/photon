#pragma once

#include "photon/Config.hpp"
#include "photon/model/Node.h"

#include <bmcl/Fwd.h>

namespace photon {

class NodeWithNamedChildren : public Node {
public:
    using Node::Node;

    virtual bmcl::OptionPtr<Node> nodeWithName(bmcl::StringView name) = 0;
};

}
