// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#pragma once

namespace onnxruntime {
namespace xnnpack {
template <typename T>
KernelCreateInfo BuildKernelCreateInfo();
}
}  // namespace onnxruntime