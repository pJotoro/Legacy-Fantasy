static uint32_t VulkanGetMemoryTypeIdx(Vulkan* vk, VkMemoryRequirements* mem_req, VkMemoryPropertyFlags properties) 
{
    uint32_t memory_type_idx;
    bool found_memory_type_idx = false;
    for (memory_type_idx = 0; memory_type_idx < vk->physical_device_memory_properties.memoryTypeCount; memory_type_idx += 1) 
    {
        if ((mem_req->memoryTypeBits & (1 << memory_type_idx)) && (vk->physical_device_memory_properties.memoryTypes[memory_type_idx].propertyFlags & properties)) 
        {
            found_memory_type_idx = true;
            break;
        }
    }
    SDL_assert(found_memory_type_idx);
    return memory_type_idx;
}

static VkPipelineShaderStageCreateInfo VulkanCreateShaderStage(VkDevice device, const char* path, VkShaderStageFlags stage) 
{
    VkPipelineShaderStageCreateInfo res = 
    {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
        .stage = stage,
        .pName = "main",
    };

    size_t len;
    void* data = SDL_LoadFile(path, &len); SDL_CHECK(data);

    VkShaderModuleCreateInfo info = 
    { 
        .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
        .codeSize = len,
        .pCode = (uint32_t*)data,
    };
    VK_CHECK(vkCreateShaderModule(device, &info, NULL, &res.module));

    SDL_free(data);
    return res;
}

static VulkanBuffer VulkanCreateBuffer(Vulkan* vk, VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags memory_properties) 
{
	VulkanBuffer res = {.size = size};
	{
		uint32_t queue_family_idx = 0; // TODO
		VkBufferCreateInfo buffer_info = 
		{
			.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
			.size = size,
			.usage = usage,

			// TODO
			.queueFamilyIndexCount = 1,
			.pQueueFamilyIndices = &queue_family_idx,
		};
		VK_CHECK(vkCreateBuffer(vk->device, &buffer_info, NULL, &res.handle));
	}
	{
		VkMemoryRequirements mem_req;
		vkGetBufferMemoryRequirements(vk->device, res.handle, &mem_req);
		uint32_t memory_type_idx = VulkanGetMemoryTypeIdx(vk, &mem_req, memory_properties);
		VkMemoryAllocateInfo mem_info = 
		{
			.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
			.allocationSize = mem_req.size,
			.memoryTypeIndex = memory_type_idx,
		};
		VK_CHECK(vkAllocateMemory(vk->device, &mem_info, NULL, &res.memory));
	}
	{
		VkDeviceSize memory_offset = 0;
		VK_CHECK(vkBindBufferMemory(vk->device, res.handle, res.memory, memory_offset));
	}
	return res;
}

static void VulkanDestroyBuffer(Vulkan* vk, VulkanBuffer* buffer) 
{
	SDL_assert(!buffer->mapped_memory);
	SDL_assert(buffer->handle);
	SDL_assert(buffer->memory);

	vkDestroyBuffer(vk->device, buffer->handle, NULL);
	vkFreeMemory(vk->device, buffer->memory, NULL);

	*buffer = (VulkanBuffer){0};
}

/**
 * If the functions below seem a little strange, that's because they are.
 * Normally, you wouldn't mess with buffer or image memory directly in this way.
 * Instead, you would use what's called a "render graph" which handles memory management for you, as well as synchronization.
 * At the same time, this was a project I made for fun, and for educational reasons, so the normal rules don't apply.

 * The basic idea is that it is never the case that you want to read from or write to a buffer at the same time.
 * For example, let's say I want to transfer some vertices to the GPU.
 * In order to do that in Vulkan, you can't *just* say glBufferData or something like you can in OpenGL (Direct3D11 has a similar thing). Instead, you have to do the following:
	
	1. Create and allocate a vertex buffer and a staging buffer, where the staging buffer has at least as much memory as the vertex buffer.
	2. Map the vertices to the staging buffer.
	3. Transfer both buffers to the GPU.
	4. Copy vertices from the staging buffer to the vertex buffer.
 
 * The above list glosses over many details, but whatever, you get the general idea.
 * To start off, in (2) it is clearly the case that there is absolutely no reason why I would want to read vertices from the staging buffer, as nothing has been written yet!
 * Furthermore, once the vertices have been written to the staging buffer, there is no reason why I'd want to write to it anymore. Writing again before first reading from it would cause the vertices I'd just written to become invalid.
 * In the same way, until I have transferred the vertices from the staging buffer to the vertex buffer, there is no reason why I'd want to read vertices from the vertex buffer.

 * Given the above, it would seem like a good idea to assign each buffer a mode.
 * What this mode is doesn't actually matter, as Vulkan will do whatever you tell it to, regardless of whether it makes sense.
 * However, for debugging purposes, we can assert that a buffer only be used for particular purposes based on which mode it is in.
 * 
 * Furthermore, we also give each buffer an 'offset' that automatically advances with each read or write, and also that determines where the next read or write will happen.
 * 'start' works the same way, except it doesn't get automatically advanced.
 * 
 * If the above explanation isn't clear, I would strongly recommend reading the code below. Having already read the above explanation, you should be able to understand it just fine.
*/

static void VulkanMapBufferMemory(Vulkan* vk, VulkanBuffer* buffer) 
{
	SDL_assert(!buffer->mapped_memory);
#if SDL_ASSERT_LEVEL >= 2
	SDL_assert(buffer->mode == VulkanBufferMode_None);
	buffer->mode = VulkanBufferMode_Write;
#endif
	VK_CHECK(vkMapMemory(vk->device, buffer->memory, buffer->start + buffer->offset, buffer->size - buffer->start, 0, &buffer->mapped_memory));
}

static void VulkanUnmapBufferMemory(Vulkan* vk, VulkanBuffer* buffer) 
{
	SDL_assert(buffer->mapped_memory);
#if SDL_ASSERT_LEVEL >= 2
	SDL_assert(buffer->mode == VulkanBufferMode_Write);
	buffer->mode = VulkanBufferMode_None;
#endif
	vkUnmapMemory(vk->device, buffer->memory);
	buffer->mapped_memory = NULL;
	buffer->offset = 0;
}

static void VulkanCopyBuffer(VkDeviceSize src_size, void* src, VulkanBuffer* buffer) 
{
	SDL_assert(src_size > 0);
	SDL_assert(buffer->mapped_memory);
	SDL_assert(buffer->offset + src_size <= buffer->size - buffer->start);
#if SDL_ASSERT_LEVEL >= 2
	SDL_assert(buffer->mode == VulkanBufferMode_Write);
#endif

	uint8_t* mapped_memory = buffer->mapped_memory;
	SDL_memcpy(mapped_memory + buffer->start + buffer->offset, src, src_size);
	buffer->offset += src_size;
}

static void VulkanCmdCopyBuffer(VkCommandBuffer cb, VulkanBuffer* src, VulkanBuffer* dst, VkDeviceSize size) 
{
#if SDL_ASSERT_LEVEL >= 2
	SDL_assert(src->mode == VulkanBufferMode_None || src->mode == VulkanBufferMode_Read);
	src->mode = VulkanBufferMode_Read;
	SDL_assert(dst->mode == VulkanBufferMode_None || dst->mode == VulkanBufferMode_Write);
	dst->mode = VulkanBufferMode_Write;
#endif
	VkBufferCopy region = 
	{
		.srcOffset = src->start + src->offset,
		.dstOffset = dst->start + dst->offset,
		.size = SDL_min(size, SDL_min(src->size - (src->start + src->offset), dst->size - (dst->start + dst->offset))),
	};
	SDL_assert(region.size != 0);
	vkCmdCopyBuffer(cb, src->handle, dst->handle, 1, &region);
	src->offset += region.size;
	dst->offset += region.size;
}

static void VulkanResetBuffer(VulkanBuffer* buffer) 
{
#if SDL_ASSERT_LEVEL >= 2
	SDL_assert(buffer->mode != VulkanBufferMode_None);
	if (buffer->mapped_memory)
	{
		buffer->mode = (buffer->mode == VulkanBufferMode_Write) ? VulkanBufferMode_Read : VulkanBufferMode_Write;
	}
	else
	{
		buffer->mode = VulkanBufferMode_None;
	}
#endif
	buffer->offset = 0;
}