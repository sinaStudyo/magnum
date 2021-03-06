#ifndef Magnum_Vk_Shader_h
#define Magnum_Vk_Shader_h
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
 * @brief Class @ref Magnum::Vk::Shader
 * @m_since_latest
 */

#include "Magnum/Magnum.h"
#include "Magnum/Tags.h"
#include "Magnum/Vk/Handle.h"
#include "Magnum/Vk/Vk.h"
#include "Magnum/Vk/Vulkan.h"
#include "Magnum/Vk/visibility.h"

namespace Magnum { namespace Vk {

/**
@brief Shader
@m_since_latest

Wraps a @type_vk_keyword{ShaderModule}.

@section Vk-Shader-creation Shader creation

The @ref ShaderCreateInfo structure takes a single required parameter, which is
the SPIR-V binary. Besides accepting a @ref Corrade::Containers::ArrayView<const void>,
to which any container is convertible, it can also take ownership of a
@ref Corrade::Containers::Array, which means you don't need to worry about
keeping a loaded file in scope until it's consumed by the @ref Shader
constructor:

@snippet MagnumVk.cpp Shader-creation
*/
class MAGNUM_VK_EXPORT Shader {
    public:
        /**
         * @brief Wrap existing Vulkan handle
         * @param device    Vulkan device the shader is created on
         * @param handle    The @type_vk{ShaderModule} handle
         * @param flags     Handle flags
         *
         * The @p handle is expected to be originating from @p device. Unlike
         * a shader created using a constructor, the Vulkan shader is by
         * default not deleted on destruction, use @p flags for different
         * behavior.
         * @see @ref release()
         */
        static Shader wrap(Device& device, VkShaderModule handle, HandleFlags flags = {});

        /**
         * @brief Constructor
         * @param device    Vulkan device to create the shader on
         * @param info      Shader creation info
         *
         * @see @fn_vk_keyword{CreateShaderModule}
         */
        explicit Shader(Device& device, const ShaderCreateInfo& info);

        /**
         * @brief Construct without creating the shader
         *
         * The constructed instance is equivalent to moved-from state. Useful
         * in cases where you will overwrite the instance later anyway. Move
         * another object over it to make it useful.
         */
        explicit Shader(NoCreateT);

        /** @brief Copying is not allowed */
        Shader(const Shader&) = delete;

        /** @brief Move constructor */
        Shader(Shader&& other) noexcept;

        /**
         * @brief Destructor
         *
         * Destroys associated @type_vk{ShaderModule} handle, unless the
         * instance was created using @ref wrap() without
         * @ref HandleFlag::DestroyOnDestruction specified.
         * @see @fn_vk_keyword{DestroyShaderModule}, @ref release()
         */
        ~Shader();

        /** @brief Copying is not allowed */
        Shader& operator=(const Shader&) = delete;

        /** @brief Move assignment */
        Shader& operator=(Shader&& other) noexcept;

        /** @brief Underlying @type_vk{Buffer} handle */
        VkShaderModule handle() { return _handle; }
        /** @overload */
        operator VkShaderModule() { return _handle; }

        /** @brief Handle flags */
        HandleFlags handleFlags() const { return _flags; }

        /**
         * @brief Release the underlying Vulkan shader
         *
         * Releases ownership of the Vulkan shader and returns its handle so
         * @fn_vk{DestroyShaderModule} is not called on destruction. The
         * internal state is then equivalent to moved-from state.
         * @see @ref wrap()
         */
        VkShaderModule release();

    private:
        /* Can't be a reference because of the NoCreate constructor */
        Device* _device;

        VkShaderModule _handle;
        HandleFlags _flags;
};

}}

#endif
