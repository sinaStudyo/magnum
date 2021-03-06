/*
    This file is part of Magnum.

    Copyright © 2010, 2011, 2012, 2013, 2014, 2015, 2016, 2017, 2018, 2019,
                2020 Vladimír Vondruš <mosra@centrum.cz>

    Permission is hereby granted, free of charge, to any person obtaining a
    copy of this software and associated documentation files (the "Software"),
    to deal in the Software without restriction, including without limitation
    the rights to use, copy, modify, merge, publish, distribute, sublicense,
    and/or sell copies of the Software, and to permit persons to whom the
    Software is furnished to do so, subject to the following conditions:

    The above copyright notice and this permission notice shall be included
    in all copies or substantial portions of the Software.

    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
    IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
    FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
    THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
    LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
    FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
    DEALINGS IN THE SOFTWARE.
*/

#include "RenderPass.h"
#include "RenderPassCreateInfo.h"

#include <Corrade/Containers/ArrayTuple.h>
#include <Corrade/Containers/GrowableArray.h>
#include <Corrade/Utility/Algorithms.h>

#include "Magnum/Vk/Assert.h"
#include "Magnum/Vk/Device.h"
#include "Magnum/Vk/Handle.h"
#include "Magnum/Vk/Image.h"
#include "Magnum/Vk/Implementation/DeviceState.h"

namespace Magnum { namespace Vk {

AttachmentDescription::AttachmentDescription(const VkFormat format, const AttachmentLoadOperation loadOperation, const AttachmentStoreOperation storeOperation, const ImageLayout initialLayout, const ImageLayout finalLayout, const Int samples, const Flags flags): _description{} {
    _description.sType = VK_STRUCTURE_TYPE_ATTACHMENT_DESCRIPTION_2;
    _description.flags = VkAttachmentDescriptionFlags(flags);
    _description.format = format;
    _description.samples = VkSampleCountFlagBits(samples);
    _description.loadOp = VkAttachmentLoadOp(loadOperation);
    _description.storeOp = VkAttachmentStoreOp(storeOperation);
    _description.initialLayout = VkImageLayout(initialLayout);
    _description.finalLayout = VkImageLayout(finalLayout);
}

AttachmentDescription::AttachmentDescription(const VkFormat format, const AttachmentLoadOperation loadOperation, const AttachmentStoreOperation storeOperation, const Int samples, const Flags flags): AttachmentDescription{format, loadOperation, storeOperation, ImageLayout::General, ImageLayout::General, samples, flags} {}

AttachmentDescription::AttachmentDescription(const VkFormat format, const std::pair<AttachmentLoadOperation, AttachmentLoadOperation> depthStencilLoadOperation, const std::pair<AttachmentStoreOperation, AttachmentStoreOperation> depthStencilStoreOperation, const ImageLayout initialLayout, const ImageLayout finalLayout, const Int samples, const Flags flags): _description{} {
    _description.sType = VK_STRUCTURE_TYPE_ATTACHMENT_DESCRIPTION_2;
    _description.flags = VkAttachmentDescriptionFlags(flags);
    _description.format = format;
    _description.samples = VkSampleCountFlagBits(samples);
    _description.loadOp = VkAttachmentLoadOp(depthStencilLoadOperation.first);
    _description.storeOp = VkAttachmentStoreOp(depthStencilStoreOperation.first);
    _description.stencilLoadOp = VkAttachmentLoadOp(depthStencilLoadOperation.second);
    _description.stencilStoreOp = VkAttachmentStoreOp(depthStencilStoreOperation.second);
    _description.initialLayout = VkImageLayout(initialLayout);
    _description.finalLayout = VkImageLayout(finalLayout);
}

AttachmentDescription::AttachmentDescription(const VkFormat format, const std::pair<AttachmentLoadOperation, AttachmentLoadOperation> depthStencilLoadOperation, const std::pair<AttachmentStoreOperation, AttachmentStoreOperation> depthStencilStoreOperation, const Int samples, const Flags flags): AttachmentDescription{format, depthStencilLoadOperation, depthStencilStoreOperation, ImageLayout::General, ImageLayout::General, samples, flags} {}

AttachmentDescription::AttachmentDescription(NoInitT) noexcept {}

AttachmentDescription::AttachmentDescription(const VkAttachmentDescription2& description):
    /* Can't use {} with GCC 4.8 here because it tries to initialize the first
       member instead of doing a copy */
    _description(description) {}

AttachmentDescription::AttachmentDescription(const VkAttachmentDescription& description): _description{
    VK_STRUCTURE_TYPE_ATTACHMENT_DESCRIPTION_2,
    nullptr,
    description.flags,
    description.format,
    description.samples,
    description.loadOp,
    description.storeOp,
    description.stencilLoadOp,
    description.stencilStoreOp,
    description.initialLayout,
    description.finalLayout
} {}

namespace {

/* Used by RenderPassCreateInfo::vkRenderPassCreateInfo() as well */
VkAttachmentDescription vkAttachmentDescription(const VkAttachmentDescription2& description) {
    return {
        description.flags,
        description.format,
        description.samples,
        description.loadOp,
        description.storeOp,
        description.stencilLoadOp,
        description.stencilStoreOp,
        description.initialLayout,
        description.finalLayout
    };
}

}

VkAttachmentDescription AttachmentDescription::vkAttachmentDescription() const {
    return Vk::vkAttachmentDescription(_description);
}

AttachmentReference::AttachmentReference(const UnsignedInt attachment, const ImageLayout layout): _reference{} {
    _reference.sType = VK_STRUCTURE_TYPE_ATTACHMENT_REFERENCE_2;
    _reference.attachment = attachment;
    _reference.layout = VkImageLayout(layout);
}

AttachmentReference::AttachmentReference(): _reference{} {
    _reference.sType = VK_STRUCTURE_TYPE_ATTACHMENT_REFERENCE_2;
    _reference.attachment = VK_ATTACHMENT_UNUSED;
    _reference.layout = VK_IMAGE_LAYOUT_UNDEFINED;
}

AttachmentReference::AttachmentReference(NoInitT) noexcept {}

AttachmentReference::AttachmentReference(const VkAttachmentReference2& reference):
    /* Can't use {} with GCC 4.8 here because it tries to initialize the first
       member instead of doing a copy */
    _reference(reference) {}

AttachmentReference::AttachmentReference(const VkAttachmentReference& reference): _reference{
    VK_STRUCTURE_TYPE_ATTACHMENT_REFERENCE_2,
    nullptr,
    reference.attachment,
    reference.layout,
    0
} {}

namespace {

/* Used in SubpassDescription::vkSubpassDescription() as well */
VkAttachmentReference vkAttachmentReference(const VkAttachmentReference2& reference) {
    return {
        reference.attachment,
        reference.layout
    };
}

}

VkAttachmentReference AttachmentReference::vkAttachmentReference() const {
    return Vk::vkAttachmentReference(_reference);
}

struct SubpassDescription::State {
    Containers::ArrayTuple inputAttachments, colorAttachments;
    AttachmentReference depthStencilAttachment;
    Containers::Array<UnsignedInt> preserveAttachments;
};

SubpassDescription::SubpassDescription(const Flags flags): _description{} {
    _description.sType = VK_STRUCTURE_TYPE_SUBPASS_DESCRIPTION_2;
    _description.flags = VkSubpassDescriptionFlags(flags);
    _description.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
}

SubpassDescription::SubpassDescription(NoInitT) noexcept {}

SubpassDescription::SubpassDescription(const VkSubpassDescription2& description):
    /* Can't use {} with GCC 4.8 here because it tries to initialize the first
       member instead of doing a copy */
    _description(description) {}

SubpassDescription::SubpassDescription(const VkSubpassDescription& description): _description{
    VK_STRUCTURE_TYPE_SUBPASS_DESCRIPTION_2,
    nullptr,
    description.flags,
    description.pipelineBindPoint,
    0,
    /* Input, color, resolve and depth/stencil attachment references set
       below */
    0, nullptr,
    0, nullptr, nullptr,
    nullptr,
    description.preserveAttachmentCount,
    description.pPreserveAttachments
}, _state{Containers::InPlaceInit} {
    /* Convert all attachment references to the "version 2" format */
    setInputAttachmentsInternal<VkAttachmentReference>({description.pInputAttachments, description.inputAttachmentCount});
    setColorAttachmentsInternal<VkAttachmentReference>({description.pColorAttachments, description.colorAttachmentCount}, {description.pResolveAttachments, description.pResolveAttachments ? description.colorAttachmentCount : 0});
    if(description.pDepthStencilAttachment)
        setDepthStencilAttachment(AttachmentReference{*description.pDepthStencilAttachment});
}

SubpassDescription::~SubpassDescription() = default;

SubpassDescription::SubpassDescription(SubpassDescription&& other) noexcept:
    /* Can't use {} with GCC 4.8 here because it tries to initialize the first
       member instead of doing a copy */
    _description(other._description),
    _state{std::move(other._state)}
{
    /* Ensure the previous instance doesn't reference state that's now ours */
    /** @todo this is now more like a destructible move, do it more selectively
        and clear only what's really ours and not external? */
    other._description.pNext = nullptr;
    other._description.inputAttachmentCount = 0;
    other._description.pInputAttachments = nullptr;
    other._description.colorAttachmentCount = 0;
    other._description.pColorAttachments = nullptr;
    other._description.pResolveAttachments = nullptr;
    other._description.pDepthStencilAttachment = nullptr;
    other._description.preserveAttachmentCount = 0;
    other._description.pPreserveAttachments = nullptr;
}

SubpassDescription& SubpassDescription::operator=(SubpassDescription&& other) noexcept {
    using std::swap;
    swap(other._description, _description);
    swap(other._state, _state);
    return *this;
}

template<class T> void SubpassDescription::setInputAttachmentsInternal(Containers::ArrayView<const T> attachments) {
    if(!_state) _state.emplace();

    Containers::ArrayView<AttachmentReference> wrappers;
    Containers::ArrayView<VkAttachmentReference2> vkAttachments2;

    _state->inputAttachments = Containers::ArrayTuple{
        {NoInit, attachments.size(), wrappers},
        {NoInit, attachments.size(), vkAttachments2}
    };

    for(std::size_t i = 0; i != attachments.size(); ++i) {
        new(wrappers + i) AttachmentReference{attachments[i]};
        /* Can't use {} with GCC 4.8 here because it tries to initialize the
           first member instead of doing a copy */
        new(vkAttachments2 + i) VkAttachmentReference2(wrappers[i]);
    }

    _description.inputAttachmentCount = attachments.size();
    _description.pInputAttachments = vkAttachments2;
}

SubpassDescription& SubpassDescription::setInputAttachments(Containers::ArrayView<const AttachmentReference> attachments) & {
    setInputAttachmentsInternal(attachments);
    return *this;
}

SubpassDescription&& SubpassDescription::setInputAttachments(Containers::ArrayView<const AttachmentReference> attachments) && {
    return std::move(setInputAttachments(attachments));
}

SubpassDescription& SubpassDescription::setInputAttachments(std::initializer_list<AttachmentReference> attachments) & {
    return setInputAttachments(Containers::arrayView(attachments));
}

SubpassDescription&& SubpassDescription::setInputAttachments(std::initializer_list<AttachmentReference> attachments) && {
    return std::move(setInputAttachments(attachments));
}

template<class T> void SubpassDescription::setColorAttachmentsInternal(Containers::ArrayView<const T> attachments, Containers::ArrayView<const T> resolveAttachments) {
    if(!_state) _state.emplace();

    CORRADE_ASSERT(!resolveAttachments.size() || resolveAttachments.size() == attachments.size(),
        "Vk::SubpassDescription::setColorAttachments(): resolve attachments expected to be either empty or have a size of" << attachments.size() << "but got" << resolveAttachments.size(), );

    Containers::ArrayView<AttachmentReference> wrappers,
        resolveWrappers;
    Containers::ArrayView<VkAttachmentReference2> vkAttachments2,
        vkResolveAttachments2;
    Containers::ArrayView<VkAttachmentReference> vkAttachments,
        vkResolveAttachments;

    _state->colorAttachments = Containers::ArrayTuple{
        {NoInit, attachments.size(), wrappers},
        {NoInit, resolveAttachments.size(), resolveWrappers},
        {NoInit, attachments.size(), vkAttachments2},
        {NoInit, resolveAttachments.size(), vkResolveAttachments2},
        {NoInit, attachments.size(), vkAttachments},
        {NoInit, resolveAttachments.size(), vkResolveAttachments}
    };

    for(std::size_t i = 0; i != attachments.size(); ++i) {
        new(wrappers + i) AttachmentReference{attachments[i]};
        /* Can't use {} with GCC 4.8 here because it tries to initialize the
           first member instead of doing a copy */
        new(vkAttachments2 + i) VkAttachmentReference2(*wrappers[i]);
        new(vkAttachments + i) VkAttachmentReference{
            vkAttachments2[i].attachment,
            vkAttachments2[i].layout
        };
    }

    if(!resolveAttachments.empty()) for(std::size_t i = 0; i != attachments.size(); ++i) {
        new(resolveWrappers + i) AttachmentReference{resolveAttachments[i]};
        /* Can't use {} with GCC 4.8 here because it tries to initialize the
           first member instead of doing a copy */
        new(vkResolveAttachments2 + i) VkAttachmentReference2(*resolveWrappers[i]);
        new(vkResolveAttachments + i) VkAttachmentReference{
            vkResolveAttachments2[i].attachment,
            vkResolveAttachments2[i].layout
        };
    }

    _description.colorAttachmentCount = attachments.size();
    _description.pColorAttachments = vkAttachments2;
    _description.pResolveAttachments = resolveAttachments.empty() ?
        nullptr : vkResolveAttachments2;
}

SubpassDescription& SubpassDescription::setColorAttachments(Containers::ArrayView<const AttachmentReference> attachments, Containers::ArrayView<const AttachmentReference> resolveAttachments) & {
    setColorAttachmentsInternal(attachments, resolveAttachments);
    return *this;
}

SubpassDescription&& SubpassDescription::setColorAttachments(Containers::ArrayView<const AttachmentReference> attachments, Containers::ArrayView<const AttachmentReference> resolveAttachments) && {
    return std::move(setColorAttachments(attachments, resolveAttachments));
}

SubpassDescription& SubpassDescription::setColorAttachments(Containers::ArrayView<const AttachmentReference> attachments) & {
    return setColorAttachments(attachments, {});
}

SubpassDescription&& SubpassDescription::setColorAttachments(Containers::ArrayView<const AttachmentReference> attachments) && {
    return std::move(setColorAttachments(attachments));
}

SubpassDescription& SubpassDescription::setColorAttachments(std::initializer_list<AttachmentReference> attachments, std::initializer_list<AttachmentReference> resolveAttachments) & {
    return setColorAttachments(Containers::arrayView(attachments), Containers::arrayView(resolveAttachments));
}

SubpassDescription&& SubpassDescription::setColorAttachments(std::initializer_list<AttachmentReference> attachments, std::initializer_list<AttachmentReference> resolveAttachments) && {
    return std::move(setColorAttachments(attachments, resolveAttachments));
}

SubpassDescription& SubpassDescription::setDepthStencilAttachment(AttachmentReference attachment) & {
    if(!_state) _state.emplace();

    _state->depthStencilAttachment = attachment;

    _description.pDepthStencilAttachment = _state->depthStencilAttachment;
    return *this;
}

SubpassDescription&& SubpassDescription::setDepthStencilAttachment(AttachmentReference attachment) && {
    return std::move(setDepthStencilAttachment(attachment));
}

SubpassDescription& SubpassDescription::setPreserveAttachments(Containers::Array<UnsignedInt>&& attachments) & {
    if(!_state) _state.emplace();

    _state->preserveAttachments = std::move(attachments);
    _description.preserveAttachmentCount = _state->preserveAttachments.size();
    _description.pPreserveAttachments = _state->preserveAttachments;
    return *this;
}

SubpassDescription&& SubpassDescription::setPreserveAttachments(Containers::Array<UnsignedInt>&& attachments) && {
    return std::move(setPreserveAttachments(std::move(attachments)));
}

SubpassDescription& SubpassDescription::setPreserveAttachments(Containers::ArrayView<const UnsignedInt> attachments) & {
    Containers::Array<UnsignedInt> copy{NoInit, attachments.size()};
    Utility::copy(attachments, copy);
    return setPreserveAttachments(std::move(copy));
}

SubpassDescription&& SubpassDescription::setPreserveAttachments(Containers::ArrayView<const UnsignedInt> attachments) && {
    return std::move(setPreserveAttachments(attachments));
}

SubpassDescription& SubpassDescription::setPreserveAttachments(std::initializer_list<UnsignedInt> attachments) & {
    return setPreserveAttachments(Containers::arrayView(attachments));
}

SubpassDescription&& SubpassDescription::setPreserveAttachments(std::initializer_list<UnsignedInt> attachments) && {
    return std::move(setPreserveAttachments(attachments));
}

namespace {

std::size_t vkSubpassDescriptionExtrasSize(const VkSubpassDescription2& description) {
    return sizeof(VkAttachmentReference)*(description.inputAttachmentCount +
        description.colorAttachmentCount*(description.pResolveAttachments ? 2 : 1) +
        (description.pDepthStencilAttachment ? 1 : 0));
}

std::pair<VkSubpassDescription, std::size_t> vkSubpassDescriptionExtrasInto(const VkSubpassDescription2& description, char* const out) {
    /* Not using an array view nor arrayCast() because the output is not
       guaranteed to be divisible by the structure size and we have nothing
       else to do with the size either */
    const auto attachmentReferences = reinterpret_cast<VkAttachmentReference*>(out);

    /* Copy what can be copied, the pointers will be filled below from the
       running offset */
    VkSubpassDescription description1{
        description.flags,
        description.pipelineBindPoint,
        description.inputAttachmentCount, nullptr,
        description.colorAttachmentCount, nullptr, nullptr,
        nullptr,
        description.preserveAttachmentCount,
        description.pPreserveAttachments
    };

    /* Save convverted attachment references to offsets inside the out view,
       update the pointers in the description structure for everything that has
       attachments */
    std::size_t offset = 0;
    if(description.inputAttachmentCount)
        description1.pInputAttachments = attachmentReferences + offset;
    for(std::size_t i = 0; i != description.inputAttachmentCount; ++i)
        attachmentReferences[offset++] = vkAttachmentReference(description.pInputAttachments[i]);
    if(description.colorAttachmentCount)
        description1.pColorAttachments = attachmentReferences + offset;
    for(std::size_t i = 0; i != description.colorAttachmentCount; ++i)
        attachmentReferences[offset++] = vkAttachmentReference(description.pColorAttachments[i]);
    if(description.pResolveAttachments) {
        description1.pResolveAttachments = attachmentReferences + offset;
        for(std::size_t i = 0; i != description.colorAttachmentCount; ++i)
            attachmentReferences[offset++] = vkAttachmentReference(description.pResolveAttachments[i]);
    }
    if(description.pDepthStencilAttachment) {
        description1.pDepthStencilAttachment = attachmentReferences + offset;
        attachmentReferences[offset++] = vkAttachmentReference(*description.pDepthStencilAttachment);
    }

    return {description1, offset*sizeof(VkAttachmentReference)};
}

}

Containers::Array<VkSubpassDescription> SubpassDescription::vkSubpassDescription() const {
    /* Allocate an array to fit VkSubpassDescription together with all
       converted VkAttachmentReference instances it needs. Expect the default
       deleter is used so we don't need to wrap some other below. */
    const std::size_t extrasSize = vkSubpassDescriptionExtrasSize(_description);
    Containers::Array<char> storage{Containers::NoInit, sizeof(VkSubpassDescription) + extrasSize};
    CORRADE_INTERNAL_ASSERT(!storage.deleter());

    /* Fill it with data and return, faking a size of 1 and with a custom
       deleter that correctly deletes as a char array again */
    std::pair<VkSubpassDescription, std::size_t> out = vkSubpassDescriptionExtrasInto(_description, storage.suffix(sizeof(VkSubpassDescription)));
    CORRADE_INTERNAL_ASSERT(out.second == extrasSize);
    *reinterpret_cast<VkSubpassDescription*>(storage.data()) = out.first;
    return Containers::Array<VkSubpassDescription>{
        reinterpret_cast<VkSubpassDescription*>(storage.release()), 1,
        [](VkSubpassDescription* data, std::size_t) {
            delete[] reinterpret_cast<char*>(data);
        }
    };
}

SubpassDependency::SubpassDependency(NoInitT) noexcept {}

SubpassDependency::SubpassDependency(const VkSubpassDependency2& dependency):
    /* Can't use {} with GCC 4.8 here because it tries to initialize the first
       member instead of doing a copy */
    _dependency(dependency) {}

SubpassDependency::SubpassDependency(const VkSubpassDependency& dependency): _dependency{
    VK_STRUCTURE_TYPE_SUBPASS_DEPENDENCY_2,
    nullptr,
    dependency.srcSubpass,
    dependency.dstSubpass,
    dependency.srcStageMask,
    dependency.dstStageMask,
    dependency.srcAccessMask,
    dependency.dstAccessMask,
    dependency.dependencyFlags,
    0
} {}

namespace {

/* Used by RenderPassCreateInfo::vkRenderPassCreateInfo() as well */
VkSubpassDependency vkSubpassDependency(const VkSubpassDependency2& dependency) {
    return {
        dependency.srcSubpass,
        dependency.dstSubpass,
        dependency.srcStageMask,
        dependency.dstStageMask,
        dependency.srcAccessMask,
        dependency.dstAccessMask,
        dependency.dependencyFlags
    };
}

}

VkSubpassDependency SubpassDependency::vkSubpassDependency() const {
    return Vk::vkSubpassDependency(_dependency);
}

struct RenderPassCreateInfo::State {
    Containers::ArrayTuple attachments;
    Containers::Array<SubpassDescription> subpasses;
    Containers::Array<VkSubpassDescription2> vkSubpasses2;
    Containers::ArrayTuple dependencies;
};

RenderPassCreateInfo::RenderPassCreateInfo(const Flags flags): _info{} {
    _info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO_2;
    _info.flags = VkRenderPassCreateFlags(flags);
}

RenderPassCreateInfo::RenderPassCreateInfo(NoInitT) noexcept {}

RenderPassCreateInfo::RenderPassCreateInfo(const VkRenderPassCreateInfo2& info):
    /* Can't use {} with GCC 4.8 here because it tries to initialize the first
       member instead of doing a copy */
    _info(info) {}

RenderPassCreateInfo::RenderPassCreateInfo(const VkRenderPassCreateInfo& info): _info{
    VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO_2,
    info.pNext, /* See note in constructor */
    info.flags,
    /* Attachment descriptions, subpass descriptions and subpass dependencies
       are set below */
    0, nullptr,
    0, nullptr,
    0, nullptr,
    /* View masks aren't present in the "version 1" structure */
    0, nullptr
} {
    /* Create attachment descriptions in the "version 2" format */
    setAttachmentsInternal<VkAttachmentDescription>({info.pAttachments, info.attachmentCount});

    /* Add subpass descriptions in the "version 1" format. Since this has to
       be done one-by-one to allow moves of SubpassDescription, no special
       internal API is needed. */
    for(std::size_t i = 0; i != info.subpassCount; ++i)
        addSubpass(SubpassDescription{info.pSubpasses[i]});

    /* Create subpass dependencies in the "version 2" format */
    setDependenciesInternal<VkSubpassDependency>({info.pDependencies, info.dependencyCount});
}

RenderPassCreateInfo::RenderPassCreateInfo(RenderPassCreateInfo&& other) noexcept:
    /* Can't use {} with GCC 4.8 here because it tries to initialize the first
       member instead of doing a copy */
    _info(other._info),
    _state{std::move(other._state)}
{
    /* Ensure the previous instance doesn't reference state that's now ours */
    /** @todo this is now more like a destructible move, do it more selectively
        and clear only what's really ours and not external? */
    other._info.pNext = nullptr;
    other._info.attachmentCount = 0;
    other._info.pAttachments = nullptr;
    other._info.subpassCount = 0;
    other._info.pSubpasses = nullptr;
    other._info.dependencyCount = 0;
    other._info.pDependencies = nullptr;
    other._info.correlatedViewMaskCount = 0;
    other._info.pCorrelatedViewMasks = nullptr;
}

RenderPassCreateInfo::~RenderPassCreateInfo() = default;

RenderPassCreateInfo& RenderPassCreateInfo::operator=(RenderPassCreateInfo&& other) noexcept {
    using std::swap;
    swap(other._info, _info);
    swap(other._state, _state);
    return *this;
}

template<class T> void RenderPassCreateInfo::setAttachmentsInternal(Containers::ArrayView<const T> attachments) {
    if(!_state) _state.emplace();

    Containers::ArrayView<AttachmentDescription> wrappers;
    Containers::ArrayView<VkAttachmentDescription2> vkAttachments2;
    _state->attachments = Containers::ArrayTuple{
        {NoInit, attachments.size(), wrappers},
        {NoInit, attachments.size(), vkAttachments2}
    };

    for(std::size_t i = 0; i != attachments.size(); ++i) {
        new(wrappers + i) AttachmentDescription{attachments[i]};
        /* Can't use {} with GCC 4.8 here because it tries to initialize the
           first member instead of doing a copy */
        new(vkAttachments2 + i) VkAttachmentDescription2(wrappers[i]);
    }

    _info.attachmentCount = attachments.size();
    _info.pAttachments = vkAttachments2;
}

RenderPassCreateInfo& RenderPassCreateInfo::setAttachments(Containers::ArrayView<const AttachmentDescription> attachments) {
    setAttachmentsInternal(attachments);
    return *this;
}

RenderPassCreateInfo& RenderPassCreateInfo::setAttachments(std::initializer_list<AttachmentDescription> attachments) {
    return setAttachments(Containers::arrayView(attachments));
}

RenderPassCreateInfo& RenderPassCreateInfo::addSubpass(SubpassDescription&& subpass) {
    if(!_state) _state.emplace();

    /* Unfortunately here we can't use an ArrayTuple as it can't grow, and
       accepting an array view / initializer list would mean a deep copy, which
       is even less acceptable. So two separate allocations it is. */
    arrayAppend(_state->subpasses, std::move(subpass));
    /* Can't use {} with GCC 4.8 here because it tries to initialize the
       first member instead of doing a copy */
    arrayAppend(_state->vkSubpasses2, VkSubpassDescription2(_state->subpasses.back()));

    /* The arrays might have been reallocated, reconnect the info structure
       pointers */
    _info.subpassCount = _state->vkSubpasses2.size();
    _info.pSubpasses = _state->vkSubpasses2;
    return *this;
}

template<class T> void RenderPassCreateInfo::setDependenciesInternal(Containers::ArrayView<const T> dependencies) {
    if(!_state) _state.emplace();

    Containers::ArrayView<SubpassDependency> wrappers;
    Containers::ArrayView<VkSubpassDependency2> vkDependencies2;
    _state->dependencies = Containers::ArrayTuple{
        {NoInit, dependencies.size(), wrappers},
        {NoInit, dependencies.size(), vkDependencies2}
    };

    for(std::size_t i = 0; i != dependencies.size(); ++i) {
        new(wrappers + i) SubpassDependency{dependencies[i]};
        /* Can't use {} with GCC 4.8 here because it tries to initialize the
           first member instead of doing a copy */
        new(vkDependencies2 + i) VkSubpassDependency2(wrappers[i]);
    }

    _info.dependencyCount = dependencies.size();
    _info.pDependencies = vkDependencies2;
}

RenderPassCreateInfo& RenderPassCreateInfo::setDependencies(Containers::ArrayView<const SubpassDependency> dependencies) {
    setDependenciesInternal(dependencies);
    return *this;
}

RenderPassCreateInfo& RenderPassCreateInfo::setDependencies(std::initializer_list<SubpassDependency> dependencies) {
    return setDependencies(Containers::arrayView(dependencies));
}

Containers::Array<VkRenderPassCreateInfo> RenderPassCreateInfo::vkRenderPassCreateInfo() const {
    /* Check that all the structs we copy into the contiguous array have the
       expected alignment requirements. Subpass descriptions have the largest
       (on 64bit) due to the internal pointers, so they'll go first. */
    static_assert(
        alignof(VkSubpassDescription) == sizeof(void*) &&
        alignof(VkAttachmentDescription) == 4 &&
        alignof(VkAttachmentReference) == 4 &&
        alignof(VkSubpassDependency) == 4,
        "unexpected alignment of VkRenderPassCreateInfo substructures");

    /* Calculate size of all "extras" */
    const std::size_t structuresSize =
        sizeof(VkSubpassDescription)*_info.subpassCount +
        sizeof(VkAttachmentDescription)*_info.attachmentCount +
        sizeof(VkSubpassDependency)*_info.dependencyCount;
    std::size_t extrasSize = 0;
    for(std::size_t i = 0; i != _info.subpassCount; ++i)
        extrasSize += vkSubpassDescriptionExtrasSize(_info.pSubpasses[i]);

    /* Allocate an array to fit VkRenderPassCreateInfo together with all extras
       it needs. Expect the default deleter is used so we don't need to wrap
       some other below. */
    Containers::Array<char> storage{Containers::NoInit, sizeof(VkRenderPassCreateInfo) + structuresSize + extrasSize};
    CORRADE_INTERNAL_ASSERT(!storage.deleter());

    /* Copy what can be copied for the output info struct. The pointers will be
       filled below from the running offset and the struct will be put into the
       storage array at the very end. */
     VkRenderPassCreateInfo info1{
        VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
        /* Right now (Vulkan 1:2.163) the set of allowed pNext structures in
           "version 2" is a *subset* of what's allowed in "version 1", so
           simply reusing them should be fine. */
        _info.pNext,
        _info.flags,
        _info.attachmentCount, nullptr,
        _info.subpassCount, nullptr,
        _info.dependencyCount, nullptr
    };

    /* Fill in the dynamically-sized subpass description structs with the
       higher alignment requirements first, put the "extras" at the end */
    std::size_t offset = sizeof(VkRenderPassCreateInfo);
    std::size_t extrasOffset = sizeof(VkRenderPassCreateInfo) + structuresSize;
    info1.pSubpasses = reinterpret_cast<VkSubpassDescription*>(storage + offset);
    for(std::size_t i = 0; i != _info.subpassCount; ++i) {
        std::pair<VkSubpassDescription, std::size_t> out = vkSubpassDescriptionExtrasInto(_info.pSubpasses[i], storage + extrasOffset);
        *reinterpret_cast<VkSubpassDescription*>(storage + offset) = out.first;
        offset += sizeof(VkSubpassDescription);
        extrasOffset += out.second;
    }
    CORRADE_INTERNAL_ASSERT(extrasOffset == storage.size());

    /* Attachment descriptions */
    info1.pAttachments = reinterpret_cast<VkAttachmentDescription*>(storage + offset);
    for(std::size_t i = 0; i != _info.attachmentCount; ++i) {
        *reinterpret_cast<VkAttachmentDescription*>(storage + offset) = vkAttachmentDescription(_info.pAttachments[i]);
        offset += sizeof(VkAttachmentDescription);
    }

    /* Subpass dependencies */
    info1.pDependencies = reinterpret_cast<VkSubpassDependency*>(storage + offset);
    for(std::size_t i = 0; i != _info.dependencyCount; ++i) {
        *reinterpret_cast<VkSubpassDependency*>(storage + offset) = vkSubpassDependency(_info.pDependencies[i]);
        offset += sizeof(VkSubpassDependency);
    }

    CORRADE_INTERNAL_ASSERT(offset == sizeof(VkRenderPassCreateInfo) + structuresSize);

    *reinterpret_cast<VkRenderPassCreateInfo*>(storage.data()) = info1;
    return Containers::Array<VkRenderPassCreateInfo>{
        reinterpret_cast<VkRenderPassCreateInfo*>(storage.release()), 1,
        [](VkRenderPassCreateInfo* data, std::size_t) {
            delete[] reinterpret_cast<char*>(data);
        }
    };
}

RenderPass RenderPass::wrap(Device& device, const VkRenderPass handle, const HandleFlags flags) {
    RenderPass out{NoCreate};
    out._device = &device;
    out._handle = handle;
    out._flags = flags;
    return out;
}

RenderPass::RenderPass(Device& device, const RenderPassCreateInfo& info):
    _device{&device},
    #ifdef CORRADE_GRACEFUL_ASSERT
    _handle{}, /* Otherwise the destructor dies if we hit the subpass assert */
    #endif
    _flags{HandleFlag::DestroyOnDestruction}
{
    CORRADE_ASSERT(info->subpassCount,
        "Vk::RenderPass: needs to be created with at least one subpass", );

    MAGNUM_VK_INTERNAL_ASSERT_SUCCESS(_device->state().createRenderPassImplementation(*_device, info, nullptr, &_handle));
}

RenderPass::RenderPass(NoCreateT): _device{}, _handle{} {}

RenderPass::RenderPass(RenderPass&& other) noexcept: _device{other._device}, _handle{other._handle}, _flags{other._flags} {
    other._handle = {};
}

RenderPass::~RenderPass() {
    if(_handle && (_flags & HandleFlag::DestroyOnDestruction))
        (**_device).DestroyRenderPass(*_device, _handle, nullptr);
}

RenderPass& RenderPass::operator=(RenderPass&& other) noexcept {
    using std::swap;
    swap(other._device, _device);
    swap(other._handle, _handle);
    swap(other._flags, _flags);
    return *this;
}

VkRenderPass RenderPass::release() {
    const VkRenderPass handle = _handle;
    _handle = {};
    return handle;
}

VkResult RenderPass::createImplementationDefault(Device& device, const RenderPassCreateInfo& info, const VkAllocationCallbacks* const callbacks, VkRenderPass* const handle) {
    return device->CreateRenderPass(device, info.vkRenderPassCreateInfo(), callbacks, handle);
}

VkResult RenderPass::createImplementationKHR(Device& device, const RenderPassCreateInfo& info, const VkAllocationCallbacks* const callbacks, VkRenderPass* const handle) {
    return device->CreateRenderPass2KHR(device, info, callbacks, handle);
}

VkResult RenderPass::createImplementation12(Device& device, const RenderPassCreateInfo& info, const VkAllocationCallbacks* const callbacks, VkRenderPass* const handle) {
    return device->CreateRenderPass2(device, info, callbacks, handle);
}

}}
