// Copyright 2017 The Dawn Authors
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

#include "dawn/native/Texture.h"

#include <algorithm>
#include <string>
#include <utility>

#include "dawn/common/Assert.h"
#include "dawn/common/Constants.h"
#include "dawn/common/Math.h"
#include "dawn/native/Adapter.h"
#include "dawn/native/ChainUtils.h"
#include "dawn/native/CommandValidation.h"
#include "dawn/native/Device.h"
#include "dawn/native/EnumMaskIterator.h"
#include "dawn/native/ObjectType_autogen.h"
#include "dawn/native/PassResourceUsage.h"
#include "dawn/native/PhysicalDevice.h"
#include "dawn/native/SharedTextureMemory.h"
#include "dawn/native/ValidationUtils_autogen.h"

namespace dawn::native {
namespace {

MaybeError ValidateTextureViewFormatCompatibility(const DeviceBase* device,
                                                  const Format& format,
                                                  wgpu::TextureFormat viewFormatEnum) {
    const Format* viewFormat;
    DAWN_TRY_ASSIGN(viewFormat, device->GetInternalFormat(viewFormatEnum));

    DAWN_INVALID_IF(!format.ViewCompatibleWith(*viewFormat),
                    "The texture view format (%s) is not texture view format compatible "
                    "with the texture format (%s).",
                    viewFormatEnum, format.format);
    return {};
}

MaybeError ValidateCanViewTextureAs(const DeviceBase* device,
                                    const TextureBase* texture,
                                    const Format& viewFormat,
                                    wgpu::TextureAspect aspect) {
    const Format& format = texture->GetFormat();

    if (aspect != wgpu::TextureAspect::All) {
        wgpu::TextureFormat aspectFormat = format.GetAspectInfo(aspect).format;
        if (viewFormat.format == aspectFormat) {
            return {};
        } else {
            return DAWN_VALIDATION_ERROR(
                "The view format (%s) is not compatible with %s of %s (%s).", viewFormat.format,
                aspect, format.format, aspectFormat);
        }
    }

    if (format.format == viewFormat.format) {
        return {};
    }

    const FormatSet& compatibleViewFormats = texture->GetViewFormats();
    if (compatibleViewFormats[viewFormat]) {
        // Validation of this list is done on texture creation, so we don't need to
        // handle the case where a format is in the list, but not compatible.
        return {};
    }

    // |viewFormat| is not in the list. Check compatibility to generate an error message
    // depending on whether it could be compatible, but needs to be explicitly listed,
    // or it could never be compatible.
    if (!format.ViewCompatibleWith(viewFormat)) {
        // The view format isn't compatible with the format at all. Return an error
        // that indicates this, in addition to reporting that it's missing from the
        // list.
        return DAWN_VALIDATION_ERROR(
            "The texture view format (%s) is not compatible with the "
            "texture format (%s)."
            "The formats must be compatible, and the view format "
            "must be passed in the list of view formats on texture creation.",
            viewFormat.format, format.format);
    }

    // The view format is compatible, but not in the list.
    return DAWN_VALIDATION_ERROR(
        "%s was not created with the texture view format (%s) "
        "in the list of compatible view formats.",
        texture, viewFormat.format);
}

bool IsTextureViewDimensionCompatibleWithTextureDimension(
    wgpu::TextureViewDimension textureViewDimension,
    wgpu::TextureDimension textureDimension) {
    switch (textureViewDimension) {
        case wgpu::TextureViewDimension::e2D:
        case wgpu::TextureViewDimension::e2DArray:
        case wgpu::TextureViewDimension::Cube:
        case wgpu::TextureViewDimension::CubeArray:
            return textureDimension == wgpu::TextureDimension::e2D;

        case wgpu::TextureViewDimension::e3D:
            return textureDimension == wgpu::TextureDimension::e3D;

        case wgpu::TextureViewDimension::e1D:
            return textureDimension == wgpu::TextureDimension::e1D;

        case wgpu::TextureViewDimension::Undefined:
            break;
    }
    UNREACHABLE();
}

bool IsArrayLayerValidForTextureViewDimension(wgpu::TextureViewDimension textureViewDimension,
                                              uint32_t textureViewArrayLayer) {
    switch (textureViewDimension) {
        case wgpu::TextureViewDimension::e2D:
        case wgpu::TextureViewDimension::e3D:
            return textureViewArrayLayer == 1u;
        case wgpu::TextureViewDimension::e2DArray:
            return true;
        case wgpu::TextureViewDimension::Cube:
            return textureViewArrayLayer == 6u;
        case wgpu::TextureViewDimension::CubeArray:
            return textureViewArrayLayer % 6 == 0;
        case wgpu::TextureViewDimension::e1D:
            return textureViewArrayLayer == 1u;

        case wgpu::TextureViewDimension::Undefined:
            break;
    }
    UNREACHABLE();
}

MaybeError ValidateSampleCount(const TextureDescriptor* descriptor,
                               wgpu::TextureUsage usage,
                               const Format* format) {
    DAWN_INVALID_IF(!IsValidSampleCount(descriptor->sampleCount),
                    "The sample count (%u) of the texture is not supported.",
                    descriptor->sampleCount);

    if (descriptor->sampleCount > 1) {
        DAWN_INVALID_IF(descriptor->mipLevelCount > 1,
                        "The mip level count (%u) of a multisampled texture is not 1.",
                        descriptor->mipLevelCount);

        // Multisampled 1D and 3D textures are not supported in D3D12/Metal/Vulkan.
        // Multisampled 2D array texture is not supported because on Metal it requires the
        // version of macOS be greater than 10.14.
        DAWN_INVALID_IF(descriptor->dimension != wgpu::TextureDimension::e2D,
                        "The dimension (%s) of a multisampled texture is not 2D.",
                        descriptor->dimension);

        DAWN_INVALID_IF(descriptor->size.depthOrArrayLayers > 1,
                        "The depthOrArrayLayers (%u) of a multisampled texture is not 1.",
                        descriptor->size.depthOrArrayLayers);

        DAWN_INVALID_IF(!format->supportsMultisample,
                        "The texture format (%s) does not support multisampling.", format->format);

        // Compressed formats are not renderable. They cannot support multisample.
        ASSERT(!format->isCompressed);

        DAWN_INVALID_IF(usage & wgpu::TextureUsage::StorageBinding,
                        "The sample count (%u) of a storage textures is not 1.",
                        descriptor->sampleCount);

        DAWN_INVALID_IF((usage & wgpu::TextureUsage::RenderAttachment) == 0,
                        "The usage (%s) of a multisampled texture doesn't include (%s).",
                        descriptor->usage, wgpu::TextureUsage::RenderAttachment);
    }

    return {};
}

MaybeError ValidateTextureViewDimensionCompatibility(const DeviceBase* device,
                                                     const TextureBase* texture,
                                                     const TextureViewDescriptor* descriptor) {
    DAWN_INVALID_IF(!IsArrayLayerValidForTextureViewDimension(descriptor->dimension,
                                                              descriptor->arrayLayerCount),
                    "The dimension (%s) of the texture view is not compatible with the layer count "
                    "(%u) of %s.",
                    descriptor->dimension, descriptor->arrayLayerCount, texture);

    DAWN_INVALID_IF(
        !IsTextureViewDimensionCompatibleWithTextureDimension(descriptor->dimension,
                                                              texture->GetDimension()),
        "The dimension (%s) of the texture view is not compatible with the dimension (%s) "
        "of %s.",
        descriptor->dimension, texture->GetDimension(), texture);

    DAWN_INVALID_IF(
        texture->GetSampleCount() > 1 && descriptor->dimension != wgpu::TextureViewDimension::e2D,
        "The dimension (%s) of the multisampled texture view is not %s.", descriptor->dimension,
        wgpu::TextureViewDimension::e2D);

    switch (descriptor->dimension) {
        case wgpu::TextureViewDimension::Cube:
        case wgpu::TextureViewDimension::CubeArray:
            DAWN_INVALID_IF(
                texture->GetSize().width != texture->GetSize().height,
                "A %s texture view is not compatible with %s because the texture's width "
                "(%u) and height (%u) are not equal.",
                descriptor->dimension, texture, texture->GetSize().width,
                texture->GetSize().height);
            DAWN_INVALID_IF(descriptor->dimension == wgpu::TextureViewDimension::CubeArray &&
                                device->IsCompatibilityMode(),
                            "A %s texture view for %s is not supported in compatibility mode",
                            descriptor->dimension, texture);
            break;
        case wgpu::TextureViewDimension::e1D:
        case wgpu::TextureViewDimension::e2D:
        case wgpu::TextureViewDimension::e2DArray:
        case wgpu::TextureViewDimension::e3D:
            break;

        case wgpu::TextureViewDimension::Undefined:
            UNREACHABLE();
    }

    return {};
}

MaybeError ValidateTextureSize(const DeviceBase* device,
                               const TextureDescriptor* descriptor,
                               const Format* format) {
    ASSERT(descriptor->size.width != 0 && descriptor->size.height != 0 &&
           descriptor->size.depthOrArrayLayers != 0);
    const CombinedLimits& limits = device->GetLimits();
    Extent3D maxExtent;
    switch (descriptor->dimension) {
        case wgpu::TextureDimension::e1D:
            maxExtent = {limits.v1.maxTextureDimension1D, 1, 1};
            break;
        case wgpu::TextureDimension::e2D:
            maxExtent = {limits.v1.maxTextureDimension2D, limits.v1.maxTextureDimension2D,
                         limits.v1.maxTextureArrayLayers};
            break;
        case wgpu::TextureDimension::e3D:
            maxExtent = {limits.v1.maxTextureDimension3D, limits.v1.maxTextureDimension3D,
                         limits.v1.maxTextureDimension3D};
            break;
    }
    DAWN_INVALID_IF(
        descriptor->size.width > maxExtent.width || descriptor->size.height > maxExtent.height ||
            descriptor->size.depthOrArrayLayers > maxExtent.depthOrArrayLayers,
        "Texture size (%s) exceeded maximum texture size (%s).", &descriptor->size, &maxExtent);

    switch (descriptor->dimension) {
        case wgpu::TextureDimension::e1D:
            DAWN_INVALID_IF(descriptor->mipLevelCount != 1,
                            "Texture mip level count (%u) is more than 1 when its dimension is %s.",
                            descriptor->mipLevelCount, wgpu::TextureDimension::e1D);
            break;
        case wgpu::TextureDimension::e2D: {
            uint32_t maxMippedDimension = std::max(descriptor->size.width, descriptor->size.height);
            DAWN_INVALID_IF(
                Log2(maxMippedDimension) + 1 < descriptor->mipLevelCount,
                "Texture mip level count (%u) exceeds the maximum (%u) for its size (%s).",
                descriptor->mipLevelCount, Log2(maxMippedDimension) + 1, &descriptor->size);
            break;
        }
        case wgpu::TextureDimension::e3D: {
            uint32_t maxMippedDimension =
                std::max(descriptor->size.width,
                         std::max(descriptor->size.height, descriptor->size.depthOrArrayLayers));
            DAWN_INVALID_IF(
                Log2(maxMippedDimension) + 1 < descriptor->mipLevelCount,
                "Texture mip level count (%u) exceeds the maximum (%u) for its size (%s).",
                descriptor->mipLevelCount, Log2(maxMippedDimension) + 1, &descriptor->size);
            break;
        }
    }

    if (format->isCompressed) {
        const TexelBlockInfo& blockInfo = format->GetAspectInfo(wgpu::TextureAspect::All).block;
        DAWN_INVALID_IF(
            descriptor->size.width % blockInfo.width != 0 ||
                descriptor->size.height % blockInfo.height != 0,
            "The size (%s) of the texture is not a multiple of the block width (%u) and "
            "height (%u) of the texture format (%s).",
            &descriptor->size, blockInfo.width, blockInfo.height, format->format);
    }

    return {};
}

MaybeError ValidateTextureUsage(const DeviceBase* device,
                                const TextureDescriptor* descriptor,
                                wgpu::TextureUsage usage,
                                const Format* format,
                                std::optional<wgpu::TextureUsage> allowedSharedTextureMemoryUsage) {
    DAWN_TRY(dawn::native::ValidateTextureUsage(usage));

    DAWN_INVALID_IF(usage == wgpu::TextureUsage::None, "The texture usage must not be 0.");

    constexpr wgpu::TextureUsage kValidCompressedUsages = wgpu::TextureUsage::TextureBinding |
                                                          wgpu::TextureUsage::CopySrc |
                                                          wgpu::TextureUsage::CopyDst;
    DAWN_INVALID_IF(
        format->isCompressed && !IsSubset(usage, kValidCompressedUsages),
        "The texture usage (%s) is incompatible with the compressed texture format (%s).", usage,
        format->format);

    DAWN_INVALID_IF(
        !format->isRenderable && (usage & wgpu::TextureUsage::RenderAttachment),
        "The texture usage (%s) includes %s, which is incompatible with the non-renderable "
        "format (%s).",
        usage, wgpu::TextureUsage::RenderAttachment, format->format);

    DAWN_INVALID_IF(descriptor->dimension == wgpu::TextureDimension::e1D &&
                        (usage & wgpu::TextureUsage::RenderAttachment),
                    "The texture usage (%s) includes %s, which is incompatible with the texture "
                    "dimension (%s).",
                    usage, wgpu::TextureUsage::RenderAttachment, descriptor->dimension);

    DAWN_INVALID_IF(!device->IsToggleEnabled(Toggle::AllowUnsafeAPIs) &&
                        descriptor->dimension == wgpu::TextureDimension::e3D &&
                        (usage & wgpu::TextureUsage::RenderAttachment),
                    "The texture dimension must not be %s for a render attachment if "
                    "allow_unsafe_apis is not enabled. See crbug.com/dawn/1020.",
                    wgpu::TextureDimension::e3D);

    DAWN_INVALID_IF(
        !format->supportsStorageUsage && (usage & wgpu::TextureUsage::StorageBinding),
        "The texture usage (%s) includes %s, which is incompatible with the format (%s).", usage,
        wgpu::TextureUsage::StorageBinding, format->format);

    const auto kTransientAttachment = wgpu::TextureUsage::TransientAttachment;
    if (usage & kTransientAttachment) {
        DAWN_INVALID_IF(
            !device->HasFeature(Feature::TransientAttachments),
            "The texture usage (%s) includes %s, which requires the %s feature to be set", usage,
            kTransientAttachment, ToAPI(Feature::TransientAttachments));

        const auto kAllowedTransientUsage =
            kTransientAttachment | wgpu::TextureUsage::RenderAttachment;
        DAWN_INVALID_IF(usage != kAllowedTransientUsage,
                        "The texture usage (%s) includes %s, which requires that the texture usage "
                        "be exactly %s",
                        usage, kTransientAttachment, kAllowedTransientUsage);
    }

    if (usage & wgpu::TextureUsage::StorageAttachment) {
        DAWN_TRY_CONTEXT(ValidateHasPLSFeature(device), "validating usage of %s",
                         wgpu::TextureUsage::StorageAttachment);

        // TODO(dawn:1704): Validate the constraints on the dimension, format, etc.
    }

    if (!allowedSharedTextureMemoryUsage) {
        // Legacy path
        // TODO(crbug.com/dawn/1795): Remove after migrating all old usages.
        // Only allows simple readonly texture usages.
        wgpu::TextureUsage validMultiPlanarUsages =
            wgpu::TextureUsage::TextureBinding | wgpu::TextureUsage::CopySrc;
        if (device->HasFeature(Feature::MultiPlanarFormatExtendedUsages)) {
            validMultiPlanarUsages |= wgpu::TextureUsage::CopyDst;
        }
        DAWN_INVALID_IF(format->IsMultiPlanar() && !IsSubset(usage, validMultiPlanarUsages),
                        "The texture usage (%s) is incompatible with the multi-planar format (%s).",
                        usage, format->format);
    } else {
        DAWN_INVALID_IF(
            !IsSubset(usage, *allowedSharedTextureMemoryUsage),
            "The texture usage (%s) is not a subset of the shared texture memory usage (%s).",
            usage, *allowedSharedTextureMemoryUsage);
    }
    return {};
}

// We need to add an internal RenderAttachment usage to some textures that has CopyDst usage as we
// apply a workaround that writes to them with a render pipeline.
bool CopyDstNeedsInternalRenderAttachmentUsage(const DeviceBase* device, const Format& format) {
    // Depth
    if (format.HasDepth() &&
        (device->IsToggleEnabled(Toggle::UseBlitForBufferToDepthTextureCopy) ||
         device->IsToggleEnabled(
             Toggle::UseBlitForDepthTextureToTextureCopyToNonzeroSubresource))) {
        return true;
    }
    // Stencil
    if (format.HasStencil() &&
        device->IsToggleEnabled(Toggle::UseBlitForBufferToStencilTextureCopy)) {
        return true;
    }
    return false;
}

// We need to add an internal TextureBinding usage to some textures that has CopySrc usage as we
// apply a workaround that binds them to a compute pipeline for their copy operation.
bool CopySrcNeedsInternalTextureBindingUsage(const DeviceBase* device, const Format& format) {
    // Snorm
    if (format.IsSnorm() && device->IsToggleEnabled(Toggle::UseBlitForSnormTextureToBufferCopy)) {
        return true;
    }
    // BGRA8Unorm
    if (format.format == wgpu::TextureFormat::BGRA8Unorm &&
        device->IsToggleEnabled(Toggle::UseBlitForBGRA8UnormTextureToBufferCopy)) {
        return true;
    }
    // Depth
    if (format.HasDepth() &&
        (device->IsToggleEnabled(Toggle::UseBlitForDepthTextureToTextureCopyToNonzeroSubresource) ||
         (format.format == wgpu::TextureFormat::Depth16Unorm &&
          device->IsToggleEnabled(Toggle::UseBlitForDepth16UnormTextureToBufferCopy)) ||
         (format.format == wgpu::TextureFormat::Depth32Float &&
          device->IsToggleEnabled(Toggle::UseBlitForDepth32FloatTextureToBufferCopy)))) {
        return true;
    }
    // Stencil
    if (format.HasStencil() &&
        device->IsToggleEnabled(Toggle::UseBlitForStencilTextureToBufferCopy)) {
        return true;
    }
    return false;
}

}  // anonymous namespace

MaybeError ValidateTextureDescriptor(
    const DeviceBase* device,
    const TextureDescriptor* descriptor,
    AllowMultiPlanarTextureFormat allowMultiPlanar,
    std::optional<wgpu::TextureUsage> allowedSharedTextureMemoryUsage) {
    DAWN_TRY(ValidateSingleSType(descriptor->nextInChain,
                                 wgpu::SType::DawnTextureInternalUsageDescriptor));

    const DawnTextureInternalUsageDescriptor* internalUsageDesc = nullptr;
    FindInChain(descriptor->nextInChain, &internalUsageDesc);

    DAWN_INVALID_IF(
        internalUsageDesc != nullptr && !device->HasFeature(Feature::DawnInternalUsages),
        "The internalUsageDesc is not empty while the dawn-internal-usages feature is not "
        "enabled");

    const Format* format;
    DAWN_TRY_ASSIGN(format, device->GetInternalFormat(descriptor->format));

    switch (allowMultiPlanar) {
        case AllowMultiPlanarTextureFormat::Yes:
            break;
        case AllowMultiPlanarTextureFormat::No:
            DAWN_INVALID_IF(format->IsMultiPlanar(),
                            "Creation of multiplanar texture format %s is not allowed.",
                            descriptor->format);
            break;
    }

    for (uint32_t i = 0; i < descriptor->viewFormatCount; ++i) {
        DAWN_TRY_CONTEXT(
            ValidateTextureViewFormatCompatibility(device, *format, descriptor->viewFormats[i]),
            "validating viewFormats[%u]", i);
    }

    wgpu::TextureUsage usage = descriptor->usage;
    if (internalUsageDesc != nullptr) {
        usage |= internalUsageDesc->internalUsage;
    }

    DAWN_TRY(ValidateTextureUsage(device, descriptor, usage, format,
                                  std::move(allowedSharedTextureMemoryUsage)));
    DAWN_TRY(ValidateTextureDimension(descriptor->dimension));
    DAWN_TRY(ValidateSampleCount(descriptor, usage, format));

    DAWN_INVALID_IF(descriptor->size.width == 0 || descriptor->size.height == 0 ||
                        descriptor->size.depthOrArrayLayers == 0 || descriptor->mipLevelCount == 0,
                    "The texture size (%s) or mipLevelCount (%u) is empty.", &descriptor->size,
                    descriptor->mipLevelCount);

    DAWN_INVALID_IF(descriptor->dimension != wgpu::TextureDimension::e2D && format->isCompressed,
                    "The dimension (%s) of a texture with a compressed format (%s) is not 2D.",
                    descriptor->dimension, format->format);

    // Depth/stencil formats are valid for 2D textures only. Metal has this limit. And D3D12
    // doesn't support depth/stencil formats on 3D textures.
    DAWN_INVALID_IF(descriptor->dimension != wgpu::TextureDimension::e2D &&
                        (format->aspects & (Aspect::Depth | Aspect::Stencil)),
                    "The dimension (%s) of a texture with a depth/stencil format (%s) is not 2D.",
                    descriptor->dimension, format->format);

    DAWN_TRY(ValidateTextureSize(device, descriptor, format));
    return {};
}

MaybeError ValidateTextureViewDescriptor(const DeviceBase* device,
                                         const TextureBase* texture,
                                         const TextureViewDescriptor* descriptor) {
    DAWN_INVALID_IF(descriptor->nextInChain != nullptr, "nextInChain must be nullptr.");

    // Parent texture should have been already validated.
    ASSERT(texture);
    ASSERT(!texture->IsError());

    DAWN_TRY(ValidateTextureViewDimension(descriptor->dimension));
    DAWN_TRY(ValidateTextureFormat(descriptor->format));
    DAWN_TRY(ValidateTextureAspect(descriptor->aspect));

    const Format& format = texture->GetFormat();
    const Format* viewFormat;
    DAWN_TRY_ASSIGN(viewFormat, device->GetInternalFormat(descriptor->format));

    DAWN_INVALID_IF(SelectFormatAspects(format, descriptor->aspect) == Aspect::None,
                    "Texture format (%s) does not have the texture view's selected aspect (%s).",
                    format.format, descriptor->aspect);

    DAWN_INVALID_IF(descriptor->arrayLayerCount == 0 || descriptor->mipLevelCount == 0,
                    "The texture view's arrayLayerCount (%u) or mipLevelCount (%u) is zero.",
                    descriptor->arrayLayerCount, descriptor->mipLevelCount);

    DAWN_INVALID_IF(
        uint64_t(descriptor->baseArrayLayer) + uint64_t(descriptor->arrayLayerCount) >
            uint64_t(texture->GetArrayLayers()),
        "Texture view array layer range (baseArrayLayer: %u, arrayLayerCount: %u) exceeds the "
        "texture's array layer count (%u).",
        descriptor->baseArrayLayer, descriptor->arrayLayerCount, texture->GetArrayLayers());

    DAWN_INVALID_IF(
        uint64_t(descriptor->baseMipLevel) + uint64_t(descriptor->mipLevelCount) >
            uint64_t(texture->GetNumMipLevels()),
        "Texture view mip level range (baseMipLevel: %u, mipLevelCount: %u) exceeds the "
        "texture's mip level count (%u).",
        descriptor->baseMipLevel, descriptor->mipLevelCount, texture->GetNumMipLevels());

    DAWN_TRY(ValidateCanViewTextureAs(device, texture, *viewFormat, descriptor->aspect));
    DAWN_TRY(ValidateTextureViewDimensionCompatibility(device, texture, descriptor));

    return {};
}

ResultOrError<TextureViewDescriptor> GetTextureViewDescriptorWithDefaults(
    const TextureBase* texture,
    const TextureViewDescriptor* descriptor) {
    ASSERT(texture);

    TextureViewDescriptor desc = {};
    if (descriptor) {
        desc = *descriptor;
    }

    // The default value for the view dimension depends on the texture's dimension with a
    // special case for 2DArray being chosen if texture is 2D but has more than one array layer.
    if (desc.dimension == wgpu::TextureViewDimension::Undefined) {
        switch (texture->GetDimension()) {
            case wgpu::TextureDimension::e1D:
                desc.dimension = wgpu::TextureViewDimension::e1D;
                break;

            case wgpu::TextureDimension::e2D:
                if (texture->GetArrayLayers() == 1) {
                    desc.dimension = wgpu::TextureViewDimension::e2D;
                } else {
                    desc.dimension = wgpu::TextureViewDimension::e2DArray;
                }
                break;

            case wgpu::TextureDimension::e3D:
                desc.dimension = wgpu::TextureViewDimension::e3D;
                break;
        }
    }

    if (desc.format == wgpu::TextureFormat::Undefined) {
        const Format& format = texture->GetFormat();

        // Check the aspect since |SelectFormatAspects| assumes a valid aspect.
        // Creation would have failed validation later since the aspect is invalid.
        DAWN_TRY(ValidateTextureAspect(desc.aspect));

        Aspect aspects = SelectFormatAspects(format, desc.aspect);
        if (HasOneBit(aspects)) {
            desc.format = format.GetAspectInfo(aspects).format;
        } else {
            desc.format = format.format;
        }
    }
    if (desc.arrayLayerCount == wgpu::kArrayLayerCountUndefined) {
        switch (desc.dimension) {
            case wgpu::TextureViewDimension::e1D:
            case wgpu::TextureViewDimension::e2D:
            case wgpu::TextureViewDimension::e3D:
                desc.arrayLayerCount = 1;
                break;
            case wgpu::TextureViewDimension::Cube:
                desc.arrayLayerCount = 6;
                break;
            case wgpu::TextureViewDimension::e2DArray:
            case wgpu::TextureViewDimension::CubeArray:
                desc.arrayLayerCount = texture->GetArrayLayers() - desc.baseArrayLayer;
                break;
            default:
                // We don't put UNREACHABLE() here because we validate enums only after this
                // function sets default values. Otherwise, the UNREACHABLE() will be hit.
                break;
        }
    }

    if (desc.mipLevelCount == wgpu::kMipLevelCountUndefined) {
        desc.mipLevelCount = texture->GetNumMipLevels() - desc.baseMipLevel;
    }
    return desc;
}

// WebGPU only supports sample counts of 1 and 4. We could expand to more based on
// platform support, but it would probably be a feature.
bool IsValidSampleCount(uint32_t sampleCount) {
    switch (sampleCount) {
        case 1:
        case 4:
            return true;

        default:
            return false;
    }
}

// TextureBase

TextureBase::TextureState::TextureState() : hasAccess(true), destroyed(false) {}

TextureBase::TextureBase(DeviceBase* device, const TextureDescriptor* descriptor)
    : ApiObjectBase(device, descriptor->label),
      mDimension(descriptor->dimension),
      mFormat(device->GetValidInternalFormat(descriptor->format)),
      mSize(descriptor->size),
      mMipLevelCount(descriptor->mipLevelCount),
      mSampleCount(descriptor->sampleCount),
      mUsage(descriptor->usage),
      mInternalUsage(mUsage),
      mFormatEnumForReflection(descriptor->format) {
    uint32_t subresourceCount = mMipLevelCount * GetArrayLayers() * GetAspectCount(mFormat.aspects);
    mIsSubresourceContentInitializedAtIndex = std::vector<bool>(subresourceCount, false);

    for (uint32_t i = 0; i < descriptor->viewFormatCount; ++i) {
        if (descriptor->viewFormats[i] == descriptor->format) {
            // Skip our own format, so the backends don't allocate the texture for
            // reinterpretation if it's not needed.
            continue;
        }
        mViewFormats[device->GetValidInternalFormat(descriptor->viewFormats[i])] = true;
    }

    const DawnTextureInternalUsageDescriptor* internalUsageDesc = nullptr;
    FindInChain(descriptor->nextInChain, &internalUsageDesc);
    if (internalUsageDesc != nullptr) {
        mInternalUsage |= internalUsageDesc->internalUsage;
    }
    GetObjectTrackingList()->Track(this);

    // dawn:1569: If a texture with multiple array layers or mip levels is specified as a
    // texture attachment when this toggle is active, it needs to be given CopyDst usage
    // internally.
    bool applyAlwaysResolveIntoZeroLevelAndLayerToggle =
        device->IsToggleEnabled(Toggle::AlwaysResolveIntoZeroLevelAndLayer) &&
        (GetArrayLayers() > 1 || GetNumMipLevels() > 1) &&
        (GetInternalUsage() & wgpu::TextureUsage::RenderAttachment);
    if (applyAlwaysResolveIntoZeroLevelAndLayerToggle) {
        AddInternalUsage(wgpu::TextureUsage::CopyDst);
    }

    if (mInternalUsage & wgpu::TextureUsage::CopyDst) {
        if (CopyDstNeedsInternalRenderAttachmentUsage(device, mFormat)) {
            AddInternalUsage(wgpu::TextureUsage::RenderAttachment);
        }
    }
    if (mInternalUsage & wgpu::TextureUsage::CopySrc) {
        if (CopySrcNeedsInternalTextureBindingUsage(device, mFormat)) {
            AddInternalUsage(wgpu::TextureUsage::TextureBinding);
        }
    }
    if (mInternalUsage & wgpu::TextureUsage::StorageBinding) {
        AddInternalUsage(kReadOnlyStorageTexture);
    }
}

TextureBase::~TextureBase() = default;

static constexpr Format kUnusedFormat;

TextureBase::TextureBase(DeviceBase* device,
                         const TextureDescriptor* descriptor,
                         ObjectBase::ErrorTag tag)
    : ApiObjectBase(device, tag, descriptor->label),
      mDimension(descriptor->dimension),
      mFormat(kUnusedFormat),
      mSize(descriptor->size),
      mMipLevelCount(descriptor->mipLevelCount),
      mSampleCount(descriptor->sampleCount),
      mUsage(descriptor->usage),
      mFormatEnumForReflection(descriptor->format) {}

void TextureBase::DestroyImpl() {
    mState.destroyed = true;

    // Destroy all of the views associated with the texture as well.
    mTextureViews.Destroy();
}

// static
TextureBase* TextureBase::MakeError(DeviceBase* device, const TextureDescriptor* descriptor) {
    return new TextureBase(device, descriptor, ObjectBase::kError);
}

ObjectType TextureBase::GetType() const {
    return ObjectType::Texture;
}

wgpu::TextureDimension TextureBase::GetDimension() const {
    ASSERT(!IsError());
    return mDimension;
}

const Format& TextureBase::GetFormat() const {
    ASSERT(!IsError());
    return mFormat;
}
const FormatSet& TextureBase::GetViewFormats() const {
    ASSERT(!IsError());
    return mViewFormats;
}
const Extent3D& TextureBase::GetSize() const {
    ASSERT(!IsError());
    return mSize;
}
uint32_t TextureBase::GetWidth() const {
    ASSERT(!IsError());
    return mSize.width;
}
uint32_t TextureBase::GetHeight() const {
    ASSERT(!IsError());
    return mSize.height;
}
uint32_t TextureBase::GetDepth() const {
    ASSERT(!IsError());
    ASSERT(mDimension == wgpu::TextureDimension::e3D);
    return mSize.depthOrArrayLayers;
}
uint32_t TextureBase::GetArrayLayers() const {
    ASSERT(!IsError());
    if (mDimension == wgpu::TextureDimension::e3D) {
        return 1;
    }
    return mSize.depthOrArrayLayers;
}
uint32_t TextureBase::GetNumMipLevels() const {
    ASSERT(!IsError());
    return mMipLevelCount;
}
SubresourceRange TextureBase::GetAllSubresources() const {
    ASSERT(!IsError());
    return {mFormat.aspects, {0, GetArrayLayers()}, {0, mMipLevelCount}};
}
uint32_t TextureBase::GetSampleCount() const {
    ASSERT(!IsError());
    return mSampleCount;
}
uint32_t TextureBase::GetSubresourceCount() const {
    ASSERT(!IsError());
    return static_cast<uint32_t>(mIsSubresourceContentInitializedAtIndex.size());
}
wgpu::TextureUsage TextureBase::GetUsage() const {
    ASSERT(!IsError());
    return mUsage;
}
wgpu::TextureUsage TextureBase::GetInternalUsage() const {
    ASSERT(!IsError());
    return mInternalUsage;
}
void TextureBase::AddInternalUsage(wgpu::TextureUsage usage) {
    ASSERT(!IsError());
    mInternalUsage |= usage;
}

bool TextureBase::IsDestroyed() const {
    ASSERT(!IsError());
    return mState.destroyed;
}

void TextureBase::SetHasAccess(bool hasAccess) {
    ASSERT(!IsError());
    mState.hasAccess = hasAccess;
}

uint32_t TextureBase::GetSubresourceIndex(uint32_t mipLevel,
                                          uint32_t arraySlice,
                                          Aspect aspect) const {
    ASSERT(HasOneBit(aspect));
    return mipLevel + GetNumMipLevels() * (arraySlice + GetArrayLayers() * GetAspectIndex(aspect));
}

bool TextureBase::IsSubresourceContentInitialized(const SubresourceRange& range) const {
    ASSERT(!IsError());
    for (Aspect aspect : IterateEnumMask(range.aspects)) {
        for (uint32_t arrayLayer = range.baseArrayLayer;
             arrayLayer < range.baseArrayLayer + range.layerCount; ++arrayLayer) {
            for (uint32_t mipLevel = range.baseMipLevel;
                 mipLevel < range.baseMipLevel + range.levelCount; ++mipLevel) {
                uint32_t subresourceIndex = GetSubresourceIndex(mipLevel, arrayLayer, aspect);
                ASSERT(subresourceIndex < mIsSubresourceContentInitializedAtIndex.size());
                if (!mIsSubresourceContentInitializedAtIndex[subresourceIndex]) {
                    return false;
                }
            }
        }
    }
    return true;
}

void TextureBase::SetIsSubresourceContentInitialized(bool isInitialized,
                                                     const SubresourceRange& range) {
    ASSERT(!IsError());
    for (Aspect aspect : IterateEnumMask(range.aspects)) {
        for (uint32_t arrayLayer = range.baseArrayLayer;
             arrayLayer < range.baseArrayLayer + range.layerCount; ++arrayLayer) {
            for (uint32_t mipLevel = range.baseMipLevel;
                 mipLevel < range.baseMipLevel + range.levelCount; ++mipLevel) {
                uint32_t subresourceIndex = GetSubresourceIndex(mipLevel, arrayLayer, aspect);
                ASSERT(subresourceIndex < mIsSubresourceContentInitializedAtIndex.size());
                mIsSubresourceContentInitializedAtIndex[subresourceIndex] = isInitialized;
            }
        }
    }
}

MaybeError TextureBase::ValidateCanUseInSubmitNow() const {
    ASSERT(!IsError());
    if (DAWN_UNLIKELY(mState.destroyed || !mState.hasAccess)) {
        DAWN_INVALID_IF(mState.destroyed, "Destroyed texture %s used in a submit.", this);
        if (DAWN_UNLIKELY(!mState.hasAccess)) {
            if (mSharedTextureMemoryState != nullptr) {
                auto memory = mSharedTextureMemoryState->GetSharedTextureMemory().Promote();
                if (memory != nullptr) {
                    return DAWN_VALIDATION_ERROR("%s used in a submit without current access to %s",
                                                 this, memory.Get());
                }
                return DAWN_VALIDATION_ERROR(
                    "%s used in a submit without current access. It's SharedTextureMemory was "
                    "already destroyed.",
                    this);
            }
            return DAWN_VALIDATION_ERROR("%s used in a submit without current access.", this);
        }
    }

    return {};
}

bool TextureBase::IsMultisampledTexture() const {
    ASSERT(!IsError());
    return mSampleCount > 1;
}

bool TextureBase::CoverFullSubresource(uint32_t mipLevel, const Extent3D& size) const {
    Extent3D levelSize = GetMipLevelSingleSubresourcePhysicalSize(mipLevel);
    switch (GetDimension()) {
        case wgpu::TextureDimension::e1D:
            return size.width == levelSize.width;
        case wgpu::TextureDimension::e2D:
            return size.width == levelSize.width && size.height == levelSize.height;
        case wgpu::TextureDimension::e3D:
            return size == levelSize;
    }
    DAWN_UNREACHABLE();
}

Extent3D TextureBase::GetMipLevelSingleSubresourceVirtualSize(uint32_t level) const {
    Extent3D extent = {std::max(mSize.width >> level, 1u), 1u, 1u};
    if (mDimension == wgpu::TextureDimension::e1D) {
        return extent;
    }

    extent.height = std::max(mSize.height >> level, 1u);
    if (mDimension == wgpu::TextureDimension::e2D) {
        return extent;
    }

    extent.depthOrArrayLayers = std::max(mSize.depthOrArrayLayers >> level, 1u);
    return extent;
}

Extent3D TextureBase::GetMipLevelSingleSubresourcePhysicalSize(uint32_t level) const {
    Extent3D extent = GetMipLevelSingleSubresourceVirtualSize(level);

    // Compressed Textures will have paddings if their width or height is not a multiple of
    // 4 at non-zero mipmap levels.
    if (mFormat.isCompressed && level != 0) {
        // If |level| is non-zero, then each dimension of |extent| is at most half of
        // the max texture dimension. Computations here which add the block width/height
        // to the extent cannot overflow.
        const TexelBlockInfo& blockInfo = mFormat.GetAspectInfo(wgpu::TextureAspect::All).block;
        extent.width = (extent.width + blockInfo.width - 1) / blockInfo.width * blockInfo.width;
        extent.height =
            (extent.height + blockInfo.height - 1) / blockInfo.height * blockInfo.height;
    }

    return extent;
}

Extent3D TextureBase::ClampToMipLevelVirtualSize(uint32_t level,
                                                 const Origin3D& origin,
                                                 const Extent3D& extent) const {
    const Extent3D virtualSizeAtLevel = GetMipLevelSingleSubresourceVirtualSize(level);
    ASSERT(origin.x <= virtualSizeAtLevel.width);
    ASSERT(origin.y <= virtualSizeAtLevel.height);
    uint32_t clampedCopyExtentWidth = (extent.width > virtualSizeAtLevel.width - origin.x)
                                          ? (virtualSizeAtLevel.width - origin.x)
                                          : extent.width;
    uint32_t clampedCopyExtentHeight = (extent.height > virtualSizeAtLevel.height - origin.y)
                                           ? (virtualSizeAtLevel.height - origin.y)
                                           : extent.height;
    return {clampedCopyExtentWidth, clampedCopyExtentHeight, extent.depthOrArrayLayers};
}

Extent3D TextureBase::GetMipLevelSubresourceVirtualSize(uint32_t level) const {
    Extent3D extent = GetMipLevelSingleSubresourceVirtualSize(level);
    if (mDimension == wgpu::TextureDimension::e2D) {
        extent.depthOrArrayLayers = mSize.depthOrArrayLayers;
    }
    return extent;
}

ResultOrError<Ref<TextureViewBase>> TextureBase::CreateView(
    const TextureViewDescriptor* descriptor) {
    return GetDevice()->CreateTextureView(this, descriptor);
}

ApiObjectList* TextureBase::GetViewTrackingList() {
    return &mTextureViews;
}

TextureViewBase* TextureBase::APICreateView(const TextureViewDescriptor* descriptor) {
    DeviceBase* device = GetDevice();

    Ref<TextureViewBase> result;
    if (device->ConsumedError(CreateView(descriptor), &result, "calling %s.CreateView(%s).", this,
                              descriptor)) {
        return TextureViewBase::MakeError(device, descriptor ? descriptor->label : nullptr);
    }
    return result.Detach();
}

bool TextureBase::IsImplicitMSAARenderTextureViewSupported() const {
    return (GetUsage() & wgpu::TextureUsage::TextureBinding) != 0;
}

SharedTextureMemoryState* TextureBase::GetSharedTextureMemoryState() const {
    return mSharedTextureMemoryState.Get();
}

void TextureBase::APIDestroy() {
    Destroy();
}

uint32_t TextureBase::APIGetWidth() const {
    return mSize.width;
}

uint32_t TextureBase::APIGetHeight() const {
    return mSize.height;
}
uint32_t TextureBase::APIGetDepthOrArrayLayers() const {
    return mSize.depthOrArrayLayers;
}

uint32_t TextureBase::APIGetMipLevelCount() const {
    return mMipLevelCount;
}

uint32_t TextureBase::APIGetSampleCount() const {
    return mSampleCount;
}

wgpu::TextureDimension TextureBase::APIGetDimension() const {
    return mDimension;
}

wgpu::TextureFormat TextureBase::APIGetFormat() const {
    return mFormatEnumForReflection;
}

wgpu::TextureUsage TextureBase::APIGetUsage() const {
    return mUsage;
}

// TextureViewBase

TextureViewBase::TextureViewBase(TextureBase* texture, const TextureViewDescriptor* descriptor)
    : ApiObjectBase(texture->GetDevice(), descriptor->label),
      mTexture(texture),
      mFormat(GetDevice()->GetValidInternalFormat(descriptor->format)),
      mDimension(descriptor->dimension),
      mRange({ConvertViewAspect(mFormat, descriptor->aspect),
              {descriptor->baseArrayLayer, descriptor->arrayLayerCount},
              {descriptor->baseMipLevel, descriptor->mipLevelCount}}) {
    GetObjectTrackingList()->Track(this);
}

TextureViewBase::TextureViewBase(DeviceBase* device, ObjectBase::ErrorTag tag, const char* label)
    : ApiObjectBase(device, tag, label), mFormat(kUnusedFormat) {}

TextureViewBase::~TextureViewBase() = default;

void TextureViewBase::DestroyImpl() {}

// static
TextureViewBase* TextureViewBase::MakeError(DeviceBase* device, const char* label) {
    return new TextureViewBase(device, ObjectBase::kError, label);
}

ObjectType TextureViewBase::GetType() const {
    return ObjectType::TextureView;
}

void TextureViewBase::FormatLabel(absl::FormatSink* s) const {
    s->Append(ObjectTypeAsString(GetType()));

    const std::string& label = GetLabel();
    if (!label.empty()) {
        s->Append(absl::StrFormat(" \"%s\"", label));
    }
    if (IsError()) {
        return;
    }

    const std::string& textureLabel = mTexture->GetLabel();
    if (!textureLabel.empty()) {
        s->Append(" of ");
        GetTexture()->FormatLabel(s);
    }
}

const TextureBase* TextureViewBase::GetTexture() const {
    ASSERT(!IsError());
    return mTexture.Get();
}

TextureBase* TextureViewBase::GetTexture() {
    ASSERT(!IsError());
    return mTexture.Get();
}

Aspect TextureViewBase::GetAspects() const {
    ASSERT(!IsError());
    return mRange.aspects;
}

const Format& TextureViewBase::GetFormat() const {
    ASSERT(!IsError());
    return mFormat;
}

wgpu::TextureViewDimension TextureViewBase::GetDimension() const {
    ASSERT(!IsError());
    return mDimension;
}

uint32_t TextureViewBase::GetBaseMipLevel() const {
    ASSERT(!IsError());
    return mRange.baseMipLevel;
}

uint32_t TextureViewBase::GetLevelCount() const {
    ASSERT(!IsError());
    return mRange.levelCount;
}

uint32_t TextureViewBase::GetBaseArrayLayer() const {
    ASSERT(!IsError());
    return mRange.baseArrayLayer;
}

uint32_t TextureViewBase::GetLayerCount() const {
    ASSERT(!IsError());
    return mRange.layerCount;
}

const SubresourceRange& TextureViewBase::GetSubresourceRange() const {
    ASSERT(!IsError());
    return mRange;
}

ApiObjectList* TextureViewBase::GetObjectTrackingList() {
    ASSERT(!IsError());
    return mTexture->GetViewTrackingList();
}

}  // namespace dawn::native
