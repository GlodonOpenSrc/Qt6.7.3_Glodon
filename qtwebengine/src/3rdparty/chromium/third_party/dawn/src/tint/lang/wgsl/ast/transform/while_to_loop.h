// Copyright 2022 The Tint Authors.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#ifndef SRC_TINT_LANG_WGSL_AST_TRANSFORM_WHILE_TO_LOOP_H_
#define SRC_TINT_LANG_WGSL_AST_TRANSFORM_WHILE_TO_LOOP_H_

#include "src/tint/lang/wgsl/ast/transform/transform.h"

namespace tint::ast::transform {

/// WhileToLoop is a Transform that converts a while statement into a loop
/// statement. This is required by the SPIR-V writer.
class WhileToLoop final : public Castable<WhileToLoop, Transform> {
  public:
    /// Constructor
    WhileToLoop();

    /// Destructor
    ~WhileToLoop() override;

    /// @copydoc Transform::Apply
    ApplyResult Apply(const Program* program,
                      const DataMap& inputs,
                      DataMap& outputs) const override;
};

}  // namespace tint::ast::transform

#endif  // SRC_TINT_LANG_WGSL_AST_TRANSFORM_WHILE_TO_LOOP_H_
