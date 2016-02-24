/*******************************************************************************
 * thrill/api/collapse.hpp
 *
 * Part of Project Thrill - http://project-thrill.org
 *
 * Copyright (C) 2015 Sebastian Lamm <seba.lamm@gmail.com>
 *
 * All rights reserved. Published under the BSD-2 license in the LICENSE file.
 ******************************************************************************/

#pragma once
#ifndef THRILL_API_COLLAPSE_HEADER
#define THRILL_API_COLLAPSE_HEADER

#include <thrill/api/dia.hpp>
#include <thrill/api/dia_node.hpp>

#include <string>
#include <vector>

namespace thrill {
namespace api {

//! \addtogroup api Interface
//! \{

template <typename ValueType, typename ParentDIA>
class CollapseNode final : public DIANode<ValueType>
{
public:
    using Super = DIANode<ValueType>;
    using Super::context_;

    /*!
     * Constructor for a LOpNode. Sets the Context, parents and stack.
     */
    explicit CollapseNode(const ParentDIA& parent)
        : Super(parent.ctx(), "Collapse", { parent.id() }, { parent.node() })
    {
        // CollapseNodes are kept by default.
        Super::consume_counter_ = Super::never_consume_;

        auto propagate_fn = [this](const ValueType& input) {
                                this->PushItem(input);
                            };
        auto lop_chain = parent.stack().push(propagate_fn).fold();
        parent.node()->AddChild(this, lop_chain);
    }

    //! A CollapseNode cannot be executed, it never contains any data.
    bool CanExecute() final { return false; }

    void Execute() final { abort(); }

    void StartPreOp(size_t /* id */) final {
        for (typename Super::Child & child : Super::children_)
            child.node->StartPreOp(child.parent_index);
    }

    void StopPreOp(size_t /* id */) final {
        for (typename Super::Child & child : Super::children_)
            child.node->StopPreOp(child.parent_index);
    }

    void PushData(bool /* consume */) final { }

    void IncConsumeCounter(size_t consume) final {
        // propagate consumption up to parents.
        for (auto& p : Super::parents_) {
            p->IncConsumeCounter(consume);
        }
    }
};

template <typename ValueType, typename Stack>
template <typename AnyStack>
DIA<ValueType, Stack>::DIA(const DIA<ValueType, AnyStack>& rhs)
// Create new CollapseNode. Transfer stack from rhs to CollapseNode. Build new
// DIA with empty stack and CollapseNode
    : DIA(std::make_shared<
              api::CollapseNode<ValueType, DIA<ValueType, AnyStack> > >(rhs)) {
    LOG0 << "WARNING: cast to DIA creates CollapseNode instead of inline chaining.";
    LOG0 << "Consider whether you can use auto instead of DIA.";
}

template <typename ValueType, typename Stack>
auto DIA<ValueType, Stack>::Collapse() const {
    assert(IsValid());

    // Create new CollapseNode. Transfer stack from rhs to CollapseNode. Build
    // new DIA with empty stack and CollapseNode
    using CollapseNode = api::CollapseNode<ValueType, DIA>;

    auto shared_node = std::make_shared<CollapseNode>(*this);

    return DIA<ValueType>(shared_node);
}

//! \}

} // namespace api
} // namespace thrill

#endif // !THRILL_API_COLLAPSE_HEADER

/******************************************************************************/
