#ifndef Magnum_Vk_Image_h
#define Magnum_Vk_Image_h
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

/** @file
 * @brief Class @ref Magnum::Vk::ImageCreateInfo, @ref Magnum::Vk::ImageCreateInfo1D, @ref Magnum::Vk::ImageCreateInfo2D, @ref Magnum::Vk::ImageCreateInfo3D, @ref Magnum::Vk::ImageCreateInfo1DArray, @ref Magnum::Vk::ImageCreateInfo2DArray, @ref Magnum::Vk::ImageCreateInfoCubeMap, @ref Magnum::Vk::ImageCreateInfoCubeMapArray, @ref Magnum::Vk::Image, enum @ref Magnum::Vk::ImageLayout, @ref Magnum::Vk::ImageUsage, enum set @ref Magnum::Vk::ImageUsages
 * @m_since_latest
 */

#include <Corrade/Containers/EnumSet.h>

#include "Magnum/DimensionTraits.h"
#include "Magnum/Magnum.h"
#include "Magnum/Math/Vector3.h"
#include "Magnum/Vk/Memory.h"
#include "Magnum/Vk/Vk.h"
#include "Magnum/Vk/Vulkan.h"
#include "Magnum/Vk/visibility.h"

namespace Magnum { namespace Vk {

namespace Implementation { struct DeviceState; }

/**
@brief Image usage
@m_since_latest

Wraps a @type_vk_keyword{ImageUsageFlagBits}.
@see @ref ImageUsages, @ref ImageCreateInfo
@m_enum_values_as_keywords
*/
enum class ImageUsage: UnsignedInt {
    /**
     * Source of a transfer command
     *
     * @see @ref ImageLayout::TransferSource
     */
    TransferSource = VK_IMAGE_USAGE_TRANSFER_SRC_BIT,

    /**
     * Destination of a transfer command
     *
     * @see @ref ImageLayout::TransferDestination
     */
    TransferDestination = VK_IMAGE_USAGE_TRANSFER_DST_BIT,

    /**
     * Sampled by a shader
     *
     * @see @ref ImageLayout::ShaderReadOnly
     */
    Sampled = VK_IMAGE_USAGE_SAMPLED_BIT,

    /** Shader storage */
    Storage = VK_IMAGE_USAGE_STORAGE_BIT,

    /**
     * Color attachment
     *
     * @see @ref ImageLayout::ColorAttachment
     */
    ColorAttachment = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,

    /**
     * Depth/stencil attachment
     *
     * @see @ref ImageLayout::DepthStencilAttachment
     */
    DepthStencilAttachment = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,

    /** Transient attachment */
    TransientAttachment = VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT,

    /**
     * Input attachment in a shader or framebuffer
     *
     * @see @ref ImageLayout::ShaderReadOnly
     */
    InputAttachment = VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT
};

/**
@brief Image usages
@m_since_latest

Type-safe wrapper for @type_vk_keyword{ImageUsageFlags}.
@see @ref ImageCreateInfo
*/
typedef Containers::EnumSet<ImageUsage> ImageUsages;

CORRADE_ENUMSET_OPERATORS(ImageUsages)

/**
@brief Image layout
@m_since_latest

@see @ref ImageCreateInfo
@m_enum_values_as_keywords
*/
enum class ImageLayout: Int {
    /**
     * Undefined. Can only be used as the initial layout in
     * @ref ImageCreateInfo structures (and there it's the default). Images in
     * this layout are not accessible by the device, the image has to be
     * transitioned to a defined layout such as @ref ImageLayout::General
     * first; contents of the memory are not guaranteed to be preserved during
     * the transition.
     * @see @ref ImageLayout::Preinitialized
     */
    Undefined = VK_IMAGE_LAYOUT_UNDEFINED,

    /**
     * Preinitialized. Can only be used as the initial layout in
     * @ref ImageCreateInfo structures. Compared to
     * @ref ImageLayout::Undefined, contents of the memory are guaranteed to be
     * preserved during a transition to a defined layout and thus this layout
     * is intended for populating image contents by the host.
     *
     * Usable only for images created with
     * @val_vk{IMAGE_TILING_LINEAR,ImageTiling}, usually with just one sample
     * and possibly other restrictions.
     *
     * @m_class{m-note m-success}
     *
     * @par
     *      In order to be populated from the host, such images need to be
     *      allocated from @ref MemoryFlag::HostVisible memory, which on
     *      discrete GPUs is not fast for device access and there's thus
     *      recommended to go through a staging buffer instead. For integrated
     *      GPUs however, going directly through a linear preinitialized image
     *      *might* be better to avoid a memory usage spike and a potentially
     *      expensive copy.
     */
    Preinitialized = VK_IMAGE_LAYOUT_PREINITIALIZED,

    /**
     * General layout, supports all types of device access. This is the
     * conservative default used everywhere except the @ref ImageCreateInfo
     * structures, which uses @ref ImageLayout::Undefined.
     *
     * @m_class{m-note m-success}
     *
     * @par
     *      While this layout will always work, it's recommended to pick a
     *      stricter layout where appropriate, as it may result in better
     *      performance.
     */
    General = VK_IMAGE_LAYOUT_GENERAL,

    /* The _OPTIMAL suffixes are dropped because it doesn't seem that there
       would be any _UNOPTIMAL or whatever variants anytime soon, so this is
       redundant. If that time comes, we can always deprecate and rename. */

    /**
     * Layout optimal for a color or resolve attachment, not guaranteed to be
     * usable for anything else.
     *
     * Only valid for images created with @ref ImageUsage::ColorAttachment.
     */
    ColorAttachment = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,

    /**
     * Layout optimal for a read/write depth/stencil attachment, not guaranteed
     * to be usable for anything else.
     *
     * Only valid for images created with
     * @ref ImageUsage::DepthStencilAttachment.
     */
    DepthStencilAttachment = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,

    /**
     * Layout optimal for read-only access in a shader sampler, combined
     * image/sampler or input attachment; not guaranteed to be usable for
     * anything else.
     *
     * Only valid for images created with @ref ImageUsage::Sampled or
     * @ref ImageUsage::InputAttachment.
     */
    ShaderReadOnly = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,

    /**
     * Layout optimal for transfer sources; not guaranteed to be usable for
     * anything else.
     *
     * Only valid for images created with @ref ImageUsage::TransferSource.
     */
    TransferSource = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,

    /**
     * Layout optimal for transfer destination; not guaranteed to be usable for
     * anything else.
     *
     * Only valid for images created with @ref ImageUsage::TransferDestination.
     */
    TransferDestination = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,

    /** @todo remaining ones from @vk_extension{KHR,maintenance2} (1.1),
        @vk_extension{KHR,separate_depth_stencil_layouts} (1.2) */
};

/**
@brief Image creation info
@m_since_latest

Wraps a @type_vk_keyword{ImageCreateInfo}. See
@ref Vk-Image-creation "Image creation" for usage information.
@see @ref ImageCreateInfo1D, @ref ImageCreateInfo2D, @ref ImageCreateInfo3D,
    @ref ImageCreateInfo1DArray, @ref ImageCreateInfo2DArray,
    @ref ImageCreateInfoCubeMap, @ref ImageCreateInfoCubeMapArray
*/
class MAGNUM_VK_EXPORT ImageCreateInfo {
    public:
        /**
         * @brief Image creation flag
         *
         * Wraps @type_vk_keyword{ImageCreateFlagBits}.
         * @see @ref Flags, @ref ImageCreateInfo(VkImageType, ImageUsages, VkFormat, const Vector3i&, Int, Int, Int, ImageLayout, Flags)
         * @m_enum_values_as_keywords
         */
        enum class Flag: UnsignedInt {
            /** @todo sparse binding/residency/aliased */

            /**
             * Allow creating a view of different format
             *
             * @todo implement @vk_extension{KHR,image_format_list}
             */
            MutableFormat = VK_IMAGE_CREATE_MUTABLE_FORMAT_BIT,

            /** Allow creating a cube map view */
            CubeCompatible = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT,

            /** @todo alias, 2D array compatible ... (Vulkan 1.1+) */
        };

        /**
         * @brief Image creation flags
         *
         * Type-safe wrapper for @type_vk_keyword{ImageCreateFlags}.
         * @see @ref ImageCreateInfo(VkImageType, ImageUsages, VkFormat, const Vector3i&, Int, Int, Int, ImageLayout, Flags),
         *      @ref ImageCreateInfo1D, @ref ImageCreateInfo2D,
         *      @ref ImageCreateInfo3D, @ref ImageCreateInfo1DArray,
         *      @ref ImageCreateInfo2DArray, @ref ImageCreateInfoCubeMap,
         *      @ref ImageCreateInfoCubeMapArray
         */
        typedef Containers::EnumSet<Flag> Flags;

        /**
         * @brief Constructor
         * @param type          Image type
         * @param usages        Desired image usage. At least one flag is
         *      required.
         * @param format        Image format
         * @param size          Image size
         * @param layers        Array layer count
         * @param levels        Mip level count
         * @param samples       Sample count
         * @param initialLayout Initial layout. Can be only either
         *      @ref ImageLayout::Undefined or @ref ImageLayout::Preinitialized.
         * @param flags         Image creation flags
         *
         * The following @type_vk{ImageCreateInfo} fields are pre-filled in
         * addition to `sType`, everything else is zero-filled:
         *
         * -    `flags`
         * -    `imageType` to @p type
         * -    `format`
         * -    `extent` to @p size
         * -    `mipLevels` to @p levels
         * -    `arrayLayers` to @p layers
         * -    `samples`
         * -    `tiling` to @val_vk{IMAGE_TILING_OPTIMAL,ImageTiling}
         * -    `usage` to @p usages
         * -    `sharingMode` to @val_vk{SHARING_MODE_EXCLUSIVE,SharingMode}
         * -    `initialLayout` to @p initialLayout
         *
         * There are various restrictions on @p size, @p layers, @p levels for
         * a particular @p type --- for common image types you're encouraged to
         * make use of @ref ImageCreateInfo1D, @ref ImageCreateInfo2D,
         * @ref ImageCreateInfo3D, @ref ImageCreateInfo1DArray,
         * @ref ImageCreateInfo2DArray, @ref ImageCreateInfoCubeMap and
         * @ref ImageCreateInfoCubeMapArray convenience classes instead of
         * this constructor.
         */
        explicit ImageCreateInfo(VkImageType type, ImageUsages usages, VkFormat format, const Vector3i& size, Int layers, Int levels, Int samples = 1, ImageLayout initialLayout = ImageLayout::Undefined, Flags flags = {});
        /* No overload w/o initialLayout here as the general public is expected
           to use the convenience classes anyway */

        /**
         * @brief Construct without initializing the contents
         *
         * Note that not even the `sType` field is set --- the structure has to
         * be fully initialized afterwards in order to be usable.
         */
        explicit ImageCreateInfo(NoInitT) noexcept;

        /**
         * @brief Construct from existing data
         *
         * Copies the existing values verbatim, pointers are kept unchanged
         * without taking over the ownership. Modifying the newly created
         * instance will not modify the original data nor the pointed-to data.
         */
        explicit ImageCreateInfo(const VkImageCreateInfo& info);

        /** @brief Underlying @type_vk{ImageCreateInfo} structure */
        VkImageCreateInfo& operator*() { return _info; }
        /** @overload */
        const VkImageCreateInfo& operator*() const { return _info; }
        /** @overload */
        VkImageCreateInfo* operator->() { return &_info; }
        /** @overload */
        const VkImageCreateInfo* operator->() const { return &_info; }
        /** @overload */
        operator const VkImageCreateInfo*() const { return &_info; }

    private:
        VkImageCreateInfo _info;
};

CORRADE_ENUMSET_OPERATORS(ImageCreateInfo::Flags)

/**
@brief Convenience constructor for 1D images
@m_since_latest

Compared to the base @ref ImageCreateInfo constructor creates an image of type
@val_vk{IMAGE_TYPE_1D,ImageType} with the last two `size` components and
`layers` set to @cpp 1 @ce.

Note that same as with the
@ref ImageCreateInfo::ImageCreateInfo(VkImageType, ImageUsages, VkFormat, const Vector3i&, Int, Int, Int, ImageLayout, Flags)
constructor, at least one @ref ImageUsage value is required.
*/
class ImageCreateInfo1D: public ImageCreateInfo {
    public:
        /** @brief Constructor */
        explicit ImageCreateInfo1D(ImageUsages usages, VkFormat format, Int size, Int levels, Int samples = 1, ImageLayout initialLayout = ImageLayout::Undefined, Flags flags = {}): ImageCreateInfo{VK_IMAGE_TYPE_1D, usages, format, {size, 1, 1}, 1, levels, samples, initialLayout, flags} {}

        /** @overload
         *
         * Equivalent to the above with @p initialLayout set to
         * @ref ImageLayout::Undefined.
         */
        explicit ImageCreateInfo1D(ImageUsages usages, VkFormat format, Int size, Int levels, Int samples, Flags flags): ImageCreateInfo1D{usages, format, size, levels, samples, ImageLayout::Undefined, flags} {}
};

/**
@brief Convenience constructor for 2D images
@m_since_latest

Compared to the base @ref ImageCreateInfo constructor creates an image of type
@val_vk{IMAGE_TYPE_2D,ImageType} with the last `size` component and `layers`
set to @cpp 1 @ce.

Note that same as with the
@ref ImageCreateInfo::ImageCreateInfo(VkImageType, ImageUsages, VkFormat, const Vector3i&, Int, Int, Int, ImageLayout, Flags)
constructor, at least one @ref ImageUsage value is required.
*/
class ImageCreateInfo2D: public ImageCreateInfo {
    public:
        /** @brief Constructor */
        explicit ImageCreateInfo2D(ImageUsages usages, VkFormat format, const Vector2i& size, Int levels, Int samples = 1, ImageLayout initialLayout = ImageLayout::Undefined, Flags flags = {}): ImageCreateInfo{VK_IMAGE_TYPE_2D, usages, format, {size, 1}, 1, levels, samples, initialLayout, flags} {}

        /** @overload
         *
         * Equivalent to the above with @p initialLayout set to
         * @ref ImageLayout::Undefined.
         */
        explicit ImageCreateInfo2D(ImageUsages usages, VkFormat format, const Vector2i& size, Int levels, Int samples, Flags flags): ImageCreateInfo2D{usages, format, size, levels, samples, ImageLayout::Undefined, flags} {}
};

/**
@brief Convenience constructor for 3D images
@m_since_latest

Compared to the base @ref ImageCreateInfo constructor creates an image of type
@val_vk{IMAGE_TYPE_3D,ImageType} with `layers` set to @cpp 1 @ce.

Note that same as with the
@ref ImageCreateInfo::ImageCreateInfo(VkImageType, ImageUsages, VkFormat, const Vector3i&, Int, Int, Int, ImageLayout, Flags)
constructor, at least one @ref ImageUsage value is required.
*/
class ImageCreateInfo3D: public ImageCreateInfo {
    public:
        /** @brief Constructor */
        explicit ImageCreateInfo3D(ImageUsages usages, VkFormat format, const Vector3i& size, Int levels, Int samples = 1, ImageLayout initialLayout = ImageLayout::Undefined, Flags flags = {}): ImageCreateInfo{VK_IMAGE_TYPE_3D, usages, format, size, 1, levels, samples, initialLayout, flags} {}

        /** @overload
         *
         * Equivalent to the above with @p initialLayout set to
         * @ref ImageLayout::Undefined.
         */
        explicit ImageCreateInfo3D(ImageUsages usages, VkFormat format, const Vector3i& size, Int levels, Int samples, Flags flags): ImageCreateInfo3D{usages, format, size, levels, samples, ImageLayout::Undefined, flags} {}
};

/**
@brief Convenience constructor for 1D array images
@m_since_latest

Compared to the base @ref ImageCreateInfo constructor creates an image of type
@val_vk{IMAGE_TYPE_1D,ImageType} with the last two `size` components set to
@cpp 1 @ce and `layers` set to @cpp size.y() @ce.

Note that same as with the
@ref ImageCreateInfo::ImageCreateInfo(VkImageType, ImageUsages, VkFormat, const Vector3i&, Int, Int, Int, ImageLayout, Flags)
constructor, at least one @ref ImageUsage value is required.
*/
class ImageCreateInfo1DArray: public ImageCreateInfo {
    public:
        /** @brief Constructor */
        explicit ImageCreateInfo1DArray(ImageUsages usages, VkFormat format, const Vector2i& size, Int levels, Int samples = 1, ImageLayout initialLayout = ImageLayout::Undefined, Flags flags = {}): ImageCreateInfo{VK_IMAGE_TYPE_1D, usages, format, {size.x(), 1, 1}, size.y(), levels, samples, initialLayout, flags} {}

        /** @overload
         *
         * Equivalent to the above with @p initialLayout set to
         * @ref ImageLayout::Undefined.
         */
        explicit ImageCreateInfo1DArray(ImageUsages usages, VkFormat format, const Vector2i& size, Int levels, Int samples, Flags flags): ImageCreateInfo1DArray{usages, format, size, levels, samples, ImageLayout::Undefined, flags} {}
};

/**
@brief Convenience constructor for 2D array images
@m_since_latest

Compared to the base @ref ImageCreateInfo constructor creates an image of type
@val_vk{IMAGE_TYPE_2D,ImageType} with the last `size` component set to
@cpp 1 @ce and `layers` set to @cpp size.z() @ce.

Note that same as with the
@ref ImageCreateInfo::ImageCreateInfo(VkImageType, ImageUsages, VkFormat, const Vector3i&, Int, Int, Int, ImageLayout, Flags)
constructor, at least one @ref ImageUsage value is required.
*/
class ImageCreateInfo2DArray: public ImageCreateInfo {
    public:
        /** @brief Constructor */
        explicit ImageCreateInfo2DArray(ImageUsages usages, VkFormat format, const Vector3i& size, Int levels, Int samples = 1, ImageLayout initialLayout = ImageLayout::Undefined, Flags flags = {}): ImageCreateInfo{VK_IMAGE_TYPE_2D, usages, format, {size.xy(), 1}, size.z(), levels, samples, initialLayout, flags} {}

        /** @overload
         *
         * Equivalent to the above with @p initialLayout set to
         * @ref ImageLayout::Undefined.
         */
        explicit ImageCreateInfo2DArray(ImageUsages usages, VkFormat format, const Vector3i& size, Int levels, Int samples, Flags flags): ImageCreateInfo2DArray{usages, format, size, levels, samples, ImageLayout::Undefined, flags} {}
};

/**
@brief Convenience constructor for cube map images
@m_since_latest

Compared to the base @ref ImageCreateInfo constructor creates an image of type
@val_vk{IMAGE_TYPE_2D,ImageType} with the last `size` component set to
@cpp 1 @ce, `layers` set to @cpp 6 @ce and `flags` additionally having
@ref Flag::CubeCompatible.

Note that same as with the
@ref ImageCreateInfo::ImageCreateInfo(VkImageType, ImageUsages, VkFormat, const Vector3i&, Int, Int, Int, ImageLayout, Flags)
constructor, at least one @ref ImageUsage value is required.
*/
class ImageCreateInfoCubeMap: public ImageCreateInfo {
    public:
        /** @brief Constructor */
        explicit ImageCreateInfoCubeMap(ImageUsages usages, VkFormat format, const Vector2i& size, Int levels, Int samples = 1, ImageLayout initialLayout = ImageLayout::Undefined, Flags flags = {}): ImageCreateInfo{VK_IMAGE_TYPE_2D, usages, format, {size, 1}, 6, levels, samples, initialLayout, flags|Flag::CubeCompatible} {}

        /** @overload
         *
         * Equivalent to the above with @p initialLayout set to
         * @ref ImageLayout::Undefined.
         */
        explicit ImageCreateInfoCubeMap(ImageUsages usages, VkFormat format, const Vector2i& size, Int levels, Int samples, Flags flags): ImageCreateInfoCubeMap{usages, format, size, levels, samples, ImageLayout::Undefined, flags} {}
};

/**
@brief Convenience constructor for cube map array images
@m_since_latest

Compared to the base @ref ImageCreateInfo constructor creates an image of type
@val_vk{IMAGE_TYPE_2D,ImageType} with the last `size` component set to
@cpp 1 @ce, `layers` set to @cpp size.y() @ce and `flags` additionally having
@ref Flag::CubeCompatible.

Note that same as with the
@ref ImageCreateInfo::ImageCreateInfo(VkImageType, ImageUsages, VkFormat, const Vector3i&, Int, Int, Int, ImageLayout, Flags)
constructor, at least one @ref ImageUsage value is required.
*/
class ImageCreateInfoCubeMapArray: public ImageCreateInfo {
    public:
        /** @brief Constructor */
        explicit ImageCreateInfoCubeMapArray(ImageUsages usages, VkFormat format, const Vector3i& size, Int levels, Int samples = 1, ImageLayout initialLayout = ImageLayout::Undefined, Flags flags = {}): ImageCreateInfo{VK_IMAGE_TYPE_2D, usages, format, {size.xy(), 1}, size.z(), levels, samples, initialLayout, flags|Flag::CubeCompatible} {}

        /** @overload
         *
         * Equivalent to the above with @p initialLayout set to
         * @ref ImageLayout::Undefined.
         */
        explicit ImageCreateInfoCubeMapArray(ImageUsages usages, VkFormat format, const Vector3i& size, Int levels, Int samples, Flags flags): ImageCreateInfoCubeMapArray{usages, format, size, levels, samples, ImageLayout::Undefined, flags} {}
};

/**
@brief Image
@m_since_latest

Wraps a @type_vk_keyword{Image} and its memory.

@section Vk-Image-creation Image creation

Pass one of the @ref ImageCreateInfo subclasses depending on desired image type
with desired usage, format, size and other propoerties to the @ref Image
constructor together with specifying @ref MemoryFlags for memory allocation.

@snippet MagnumVk.cpp Image-creation

@attention At this point, a dedicated allocation is used, subsequently
    accessible through @ref dedicatedMemory(). This behavior may change in the
    future.

@subsection Vk-Image-creation-custom-allocation Custom memory allocation

Using @ref Image(Device&, const ImageCreateInfo&, NoAllocateT), the image will
be created without any memory attached. Image memory requirements can be
subsequently queried using @ref memoryRequirements() and an allocated memory
bound with @ref bindMemory(). See @ref Memory for further details about memory allocation.

@snippet MagnumVk.cpp Image-creation-custom-allocation

Using @ref bindDedicatedMemory() instead of @ref bindMemory() will transfer
ownership of the @ref Memory to the image instance, making it subsequently
available through @ref dedicatedMemory(). This matches current behavior of the
@ref Image(Device&, const ImageCreateInfo&, MemoryFlags) constructor shown
above, except that you have more control over choosing and allocating the
memory.

@see @ref Buffer
*/
class MAGNUM_VK_EXPORT Image {
    public:
        /**
         * @brief Wrap existing Vulkan handle
         * @param device    Vulkan device the image is created on
         * @param handle    The @type_vk{Image} handle
         * @param flags     Handle flags
         *
         * The @p handle is expected to be originating from @p device. Unlike
         * an image created using a constructor, the Vulkan image is by default
         * not deleted on destruction, use @p flags for different behavior.
         * @see @ref release()
         */
        static Image wrap(Device& device, VkImage handle, HandleFlags flags = {});

        /**
         * @brief Construct an image without allocating
         * @param device    Vulkan device to create the image on
         * @param info      Image creation info
         *
         * Use @ref memoryRequirements(), @ref Memory and @ref bindMemory() to
         * bind a memory (sub)allocation to the image.
         * @see @ref Image(Device&, const ImageCreateInfo&, MemoryFlags),
         *      @fn_vk_keyword{CreateImage}
         */
        explicit Image(Device& device, const ImageCreateInfo& info, NoAllocateT);

        /**
         * @brief Construct an image
         * @param device        Vulkan device to create the image on
         * @param info          Image creation info
         * @param memoryFlags   Memory allocation flags
         *
         * Compared to @ref Image(Device&, const ImageCreateInfo&, NoAllocateT)
         * allocates a memory satisfying @p memoryFlags as well.
         *
         * @attention At this point, a dedicated allocation is used,
         *      subsequently accessible through @ref dedicatedMemory(). This
         *      behavior may change in the future.
         */
        explicit Image(Device& device, const ImageCreateInfo& info, MemoryFlags memoryFlags);

        /**
         * @brief Construct without creating the image
         *
         * The constructed instance is equivalent to moved-from state. Useful
         * in cases where you will overwrite the instance later anyway. Move
         * another object over it to make it useful.
         */
        explicit Image(NoCreateT);

        /** @brief Copying is not allowed */
        Image(const Image&) = delete;

        /** @brief Move constructor */
        Image(Image&& other) noexcept;

        /**
         * @brief Destructor
         *
         * Destroys associated @type_vk{Image} handle, unless the instance
         * was created using @ref wrap() without @ref HandleFlag::DestroyOnDestruction
         * specified.
         * @see @fn_vk_keyword{DestroyImage}, @ref release()
         */
        ~Image();

        /** @brief Copying is not allowed */
        Image& operator=(const Image&) = delete;

        /** @brief Move assignment */
        Image& operator=(Image&& other) noexcept;

        /** @brief Underlying @type_vk{Image} handle */
        VkImage handle() { return _handle; }
        /** @overload */
        operator VkImage() { return _handle; }

        /** @brief Handle flags */
        HandleFlags handleFlags() const { return _flags; }

        /**
         * @brief Image memory requirements
         *
         * @see @ref bindMemory(), @fn_vk_keyword{GetImageMemoryRequirements2},
         *      @fn_vk_keyword{GetImageMemoryRequirements}
         */
        MemoryRequirements memoryRequirements() const;

        /**
         * @brief Bind image memory
         *
         * Assumes that @p memory type, the amount of @p memory at @p offset
         * and @p offset alignment corresponds to image memory requirements.
         * @see @ref memoryRequirements(), @ref bindDedicatedMemory(),
         *      @fn_vk_keyword{BindImageMemory2},
         *      @fn_vk_keyword{BindImageMemory}
         */
        void bindMemory(Memory& memory, UnsignedLong offset);

        /**
         * @brief Bind a dedicated image memory
         *
         * Equivalent to @ref bindMemory() with @p offset set to @cpp 0 @ce,
         * with the additional effect that @p memory ownership transfers to the
         * image and is then available through @ref dedicatedMemory().
         */
        void bindDedicatedMemory(Memory&& memory);

        /**
         * @brief Whether the image has a dedicated memory
         *
         * Returns @cpp true @ce if the image memory was bound using
         * @ref bindDedicatedMemory(), @cpp false @ce otherwise.
         * @see @ref dedicatedMemory()
         */
        bool hasDedicatedMemory() const;

        /**
         * @brief Dedicated image memory
         *
         * Expects that the image has a dedicated memory.
         * @see @ref hasDedicatedMemory()
         */
        Memory& dedicatedMemory();

        /**
         * @brief Release the underlying Vulkan image
         *
         * Releases ownership of the Vulkan image and returns its handle so
         * @fn_vk{DestroyImage} is not called on destruction. The internal
         * state is then equivalent to moved-from state.
         * @see @ref wrap()
         */
        VkImage release();

    private:
        friend Implementation::DeviceState;

        MAGNUM_VK_LOCAL static void getMemoryRequirementsImplementationDefault(Device& device, const VkImageMemoryRequirementsInfo2& info, VkMemoryRequirements2& requirements);
        MAGNUM_VK_LOCAL static void getMemoryRequirementsImplementationKHR(Device& device, const VkImageMemoryRequirementsInfo2& info, VkMemoryRequirements2& requirements);
        MAGNUM_VK_LOCAL static void getMemoryRequirementsImplementation11(Device& device, const VkImageMemoryRequirementsInfo2& info, VkMemoryRequirements2& requirements);

        MAGNUM_VK_LOCAL static VkResult bindMemoryImplementationDefault(Device& device, UnsignedInt count, const VkBindImageMemoryInfo* infos);
        MAGNUM_VK_LOCAL static VkResult bindMemoryImplementationKHR(Device& device, UnsignedInt count, const VkBindImageMemoryInfo* infos);
        MAGNUM_VK_LOCAL static VkResult bindMemoryImplementation11(Device& device, UnsignedInt count, const VkBindImageMemoryInfo* infos);

        /* Can't be a reference because of the NoCreate constructor */
        Device* _device;

        VkImage _handle;
        HandleFlags _flags;
        Memory _dedicatedMemory;
};

}}

#endif
