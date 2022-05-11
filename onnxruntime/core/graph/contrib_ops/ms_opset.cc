// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "ms_opset.h"

#include "onnx/defs/operator_sets.h"

#include "core/graph/constants.h"
#include "core/graph/contrib_ops/nhwc_inference_context.h"

namespace onnxruntime {
namespace contrib {

template <typename F>
void RegisterNHWCSchema(F&& f, ::ONNX_NAMESPACE::OpSchema&& schema) {
  // Need to copy the inferencing function from the temporary OpSchema object
  auto onnx_inferencing_func = schema.GetTypeAndShapeInferenceFunction();
  f(std::move(::ONNX_NAMESPACE::OpSchema(schema)
                  .TypeAndShapeInferenceFunction([onnx_inferencing_func](ONNX_NAMESPACE::InferenceContext& ctx) {
                    // use the NHWC inferencing context to convert input 0 and output 0 to NCHW
                    // so the ONNX shape inferencing can be used. Once that completes, the call to PropagateOutputShape
                    // will convert the inferred shape from NCHW to NHWC
                    NhwcInferenceContext nhwc_ctx(ctx);
                    onnx_inferencing_func(nhwc_ctx);
                    nhwc_ctx.PropagateOutputShape();
                  })
                  .SetDomain(onnxruntime::kMSInternalNHWCDomain)));
}

template <typename F>
void RegisterNHWCConvWithActivation(F&& f, ::ONNX_NAMESPACE::OpSchema&& schema) {
  // Need to copy the inferencing function from the temporary OpSchema object
  auto onnx_inferencing_func = schema.GetTypeAndShapeInferenceFunction();
  f(std::move(::ONNX_NAMESPACE::OpSchema(schema)
                  .Attr("activation", "", ONNX_NAMESPACE::AttributeProto::STRING, ONNX_NAMESPACE::OPTIONAL_VALUE)
                  .Attr("activation_params", "", ONNX_NAMESPACE::AttributeProto::FLOATS, ONNX_NAMESPACE::OPTIONAL_VALUE)
                  .TypeAndShapeInferenceFunction([onnx_inferencing_func](ONNX_NAMESPACE::InferenceContext& ctx) {
                    // use the NHWC inferencing context to convert input 0 and output 0 to NCHW
                    // so the ONNX shape inferencing can be used. Once that completes, the call to PropagateOutputShape
                    // will convert the inferred shape from NCHW to NHWC
                    NhwcInferenceContext nhwc_ctx(ctx);
                    onnx_inferencing_func(nhwc_ctx);
                    nhwc_ctx.PropagateOutputShape();
                  })
                  .SetDomain(onnxruntime::kMSInternalNHWCDomain)));
}

#define REGISTER_NHWC_SCHEMA(Op, SinceVersion) \
  { RegisterNHWCSchema(                        \
      fn,                                      \
      ::ONNX_NAMESPACE::GetOpSchema<           \
          ::ONNX_NAMESPACE::ONNX_OPERATOR_SET_SCHEMA_CLASS_NAME(Onnx, SinceVersion, Op)>()); }

#define REGISTER_NHWC_SCHEMA_WITH_ACTIVATION(Op, SinceVersion) \
  { RegisterNHWCConvWithActivation(                            \
      fn,                                                      \
      ::ONNX_NAMESPACE::GetOpSchema<                           \
          ::ONNX_NAMESPACE::ONNX_OPERATOR_SET_SCHEMA_CLASS_NAME(Onnx, SinceVersion, Op)>()); }

// Schemas for ops that are NHWC versions of ONNX operators. They are created by the layout transformer by converting
// the relevant input/outputs of a node between NCHW and NHWC, and moving the node to the kMSInternalNHWCDomain domain.
void OpSet_Internal_NHWC_ver1::ForEachSchema(std::function<void(ONNX_NAMESPACE::OpSchema&&)> fn) {
  // if the operator may be fused with an activation, use the WITH_ACTIVATION variant to add optional attributes
  // for the activation parameters.
  REGISTER_NHWC_SCHEMA_WITH_ACTIVATION(Conv, 1);
  REGISTER_NHWC_SCHEMA_WITH_ACTIVATION(Conv, 11);
  REGISTER_NHWC_SCHEMA_WITH_ACTIVATION(MaxPool, 1);
  REGISTER_NHWC_SCHEMA_WITH_ACTIVATION(MaxPool, 8);
  REGISTER_NHWC_SCHEMA_WITH_ACTIVATION(MaxPool, 10);
  REGISTER_NHWC_SCHEMA_WITH_ACTIVATION(MaxPool, 11);
  REGISTER_NHWC_SCHEMA_WITH_ACTIVATION(MaxPool, 12);

  // TODO: Add other layout sensitive ops when needed. Those are:
  //   QLinearConv,
  //   BatchNormalization,
  //   AveragePool, GlobalAveragePool, GlobalMaxPool,
  //   LRN,
  //   GridSample
  //   DepthToSpace, SpaceToDepth
}

}  // namespace contrib
}  // namespace onnxruntime