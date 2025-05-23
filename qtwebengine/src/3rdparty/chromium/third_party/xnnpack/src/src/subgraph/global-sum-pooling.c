// Copyright 2020 Google LLC
//
// This source code is licensed under the BSD-style license found in the
// LICENSE file in the root directory of this source tree.

#include <assert.h>
#include <math.h>
#include <stddef.h>
#include <stdint.h>

#include <xnnpack.h>
#include <xnnpack/log.h>
#include <xnnpack/operator.h>
#include <xnnpack/params.h>
#include <xnnpack/requantization.h>
#include <xnnpack/subgraph.h>
#include <xnnpack/subgraph-validation.h>


static enum xnn_status create_global_sum_pooling_operator(
  const struct xnn_node* node,
  const struct xnn_value* values,
  size_t num_values,
  struct xnn_operator_data* opdata,
  const struct xnn_caches* caches)
{
  assert(node->num_inputs == 1);
  const uint32_t input_id = node->inputs[0];
  assert(input_id != XNN_INVALID_VALUE_ID);
  assert(input_id < num_values);

  assert(node->num_outputs == 1);
  const uint32_t output_id = node->outputs[0];
  assert(output_id != XNN_INVALID_VALUE_ID);
  assert(output_id < num_values);

  const size_t num_input_dims = values[input_id].shape.num_dims;
  assert(num_input_dims >= 1);
  const size_t channel_dim = values[input_id].shape.dim[num_input_dims - 1];

  enum xnn_status status;
  assert(values[node->inputs[0]].layout == xnn_layout_type_nhwc);
  assert(values[node->outputs[0]].layout == xnn_layout_type_nhwc);
  switch (node->compute_type) {
    case xnn_compute_type_fp32:
      status = xnn_create_global_sum_pooling_nwc_f32(
          channel_dim /* channels */, channel_dim /* input stride */, channel_dim /* output stride */,
          node->activation.output_min,
          node->activation.output_max,
          node->flags,
          &opdata->operator_objects[0]);
      break;
    case xnn_compute_type_fp16:
      status = xnn_create_global_sum_pooling_nwc_f16(
          channel_dim /* channels */, channel_dim /* input stride */, channel_dim /* output stride */,
          node->activation.output_min,
          node->activation.output_max,
          node->flags,
          &opdata->operator_objects[0]);
      break;
    default:
      XNN_UNREACHABLE;
  }
  if (status == xnn_status_success) {
    switch (node->type) {
      case xnn_node_type_global_sum_pooling_1d:
        opdata->batch_size = xnn_shape_multiply_batch_dims(&values[input_id].shape, 2);
        opdata->input_width = values[input_id].shape.dim[num_input_dims - 2];
        break;
      case xnn_node_type_global_sum_pooling_2d:
        opdata->batch_size = xnn_shape_multiply_batch_dims(&values[input_id].shape, 3);
        opdata->input_width = values[input_id].shape.dim[num_input_dims - 3] * values[input_id].shape.dim[num_input_dims - 2];
        break;
      default:
        XNN_UNREACHABLE;
    }
    opdata->inputs[0] = input_id;
    opdata->outputs[0] = output_id;
  }
  return status;
}

static enum xnn_status setup_global_sum_pooling_operator(
  const struct xnn_operator_data* opdata,
  const struct xnn_blob* blobs,
  size_t num_blobs,
  pthreadpool_t threadpool)
{
  const uint32_t input_id = opdata->inputs[0];
  assert(input_id != XNN_INVALID_VALUE_ID);
  assert(input_id < num_blobs);

  const uint32_t output_id = opdata->outputs[0];
  assert(output_id != XNN_INVALID_VALUE_ID);
  assert(output_id < num_blobs);

  const struct xnn_blob* input_blob = blobs + input_id;
  const void* input_data = input_blob->data;
  assert(input_data != NULL);

  const struct xnn_blob* output_blob = blobs + output_id;
  void* output_data = output_blob->data;
  assert(output_data != NULL);

  switch (opdata->operator_objects[0]->type) {
    case xnn_operator_type_global_sum_pooling_nwc_f32:
      return xnn_setup_global_sum_pooling_nwc_f32(
        opdata->operator_objects[0],
        opdata->batch_size,
        opdata->input_width,
        input_data,
        output_data,
        threadpool);
      break;
    case xnn_operator_type_global_sum_pooling_nwc_f16:
      return xnn_setup_global_sum_pooling_nwc_f16(
        opdata->operator_objects[0],
        opdata->batch_size,
        opdata->input_width,
        input_data,
        output_data,
        threadpool);
      break;
    default:
      XNN_UNREACHABLE;
  }
}

static enum xnn_status define_global_sum_pooling_nd(
  xnn_subgraph_t subgraph,
  enum xnn_node_type node_type,
  float output_min,
  float output_max,
  uint32_t input_id,
  uint32_t output_id,
  uint32_t flags)
{
  enum xnn_status status;
  if ((status = xnn_subgraph_check_xnnpack_initialized(node_type)) != xnn_status_success) {
    return status;
  }

  status = xnn_subgraph_check_output_min_max(node_type, output_min, output_max);
  if (status != xnn_status_success) {
    return status;
  }

  status = xnn_subgraph_check_input_node_id(node_type, input_id, subgraph->num_values);
  if (status != xnn_status_success) {
    return status;
  }

  const struct xnn_value* input_value = &subgraph->values[input_id];
  status = xnn_subgraph_check_input_type_dense(node_type, input_id, input_value);
  if (status != xnn_status_success) {
    return status;
  }

  switch (input_value->datatype) {
    case xnn_datatype_fp32:
      break;
    default:
      xnn_log_error(
        "failed to define %s operator with input ID #%" PRIu32 ": unsupported Value datatype %s (%d)",
        xnn_node_type_to_string(node_type), input_id,
        xnn_datatype_to_string(input_value->datatype), input_value->datatype);
      return xnn_status_invalid_parameter;
  }

  status = xnn_subgraph_check_output_node_id(node_type, output_id, subgraph->num_values);
  if (status != xnn_status_success) {
    return status;
  }

  const struct xnn_value* output_value = &subgraph->values[output_id];
  status = xnn_subgraph_check_output_type_dense(node_type, output_id, output_value);
  if (status != xnn_status_success) {
    return status;
  }

  enum xnn_compute_type compute_type = xnn_compute_type_invalid;
  switch (output_value->datatype) {
    case xnn_datatype_fp32:
      compute_type = xnn_compute_type_fp32;
      break;
    default:
      xnn_log_error(
        "failed to define %s operator with output ID #%" PRIu32 ": unsupported Value datatype %s (%d)",
        xnn_node_type_to_string(node_type), output_id,
        xnn_datatype_to_string(output_value->datatype), output_value->datatype);
      return xnn_status_invalid_parameter;
  }

  status = xnn_subgraph_check_datatype_matches(
    node_type, input_id, input_value, output_id, output_value);
  if (status != xnn_status_success) {
    return status;
  }

  struct xnn_node* node = xnn_subgraph_new_node(subgraph);
  if (node == NULL) {
    return xnn_status_out_of_memory;
  }

  node->type = node_type;
  node->compute_type = compute_type;
  node->activation.output_min = output_min;
  node->activation.output_max = output_max;
  node->num_inputs = 1;
  node->inputs[0] = input_id;
  node->num_outputs = 1;
  node->outputs[0] = output_id;
  node->flags = flags;

  node->create = create_global_sum_pooling_operator;
  node->setup = setup_global_sum_pooling_operator;

  return xnn_status_success;
}

enum xnn_status xnn_define_global_sum_pooling_1d(
  xnn_subgraph_t subgraph,
  float output_min,
  float output_max,
  uint32_t input_id,
  uint32_t output_id,
  uint32_t flags)
{
  return define_global_sum_pooling_nd(
    subgraph, xnn_node_type_global_sum_pooling_1d, output_min, output_max, input_id, output_id, flags);
}

enum xnn_status xnn_define_global_sum_pooling_2d(
  xnn_subgraph_t subgraph,
  float output_min,
  float output_max,
  uint32_t input_id,
  uint32_t output_id,
  uint32_t flags)
{
  return define_global_sum_pooling_nd(
    subgraph, xnn_node_type_global_sum_pooling_2d, output_min, output_max, input_id, output_id, flags);
}
