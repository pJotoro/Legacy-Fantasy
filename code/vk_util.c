function uint32_t VulkanGetMemoryTypeIdx(Vulkan* vk, VkMemoryRequirements* mem_req, VkMemoryPropertyFlags properties) {
    uint32_t memory_type_idx;
    bool found_memory_type_idx = false;
    for (memory_type_idx = 0; memory_type_idx < vk->physical_device_memory_properties.memoryTypeCount; memory_type_idx += 1) {
        if ((mem_req->memoryTypeBits & (1 << memory_type_idx)) && (vk->physical_device_memory_properties.memoryTypes[memory_type_idx].propertyFlags & properties)) {
            found_memory_type_idx = true;
            break;
        }
    }
    SDL_assert(found_memory_type_idx);
    return memory_type_idx;
}

function VkPipelineShaderStageCreateInfo VulkanCreateShaderStage(VkDevice device, const char* path, VkShaderStageFlags stage) {
    VkPipelineShaderStageCreateInfo res = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
        .stage = stage,
        .pName = "main",
    };

    size_t len;
    void* data = SDL_LoadFile(path, &len); SDL_CHECK(data);

    VkShaderModuleCreateInfo info = { 
        .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
        .codeSize = len,
        .pCode = (uint32_t*)data,
    };
    VK_CHECK(vkCreateShaderModule(device, &info, NULL, &res.module));

    SDL_free(data);
    return res;
}

function VulkanBuffer VulkanCreateBuffer(Vulkan* vk, VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags memory_properties) {
	VulkanBuffer res = {.size = size};
	{
		uint32_t queue_family_idx = 0; // TODO
		VkBufferCreateInfo buffer_info = {
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
		VkMemoryAllocateInfo mem_info = {
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

function void VulkanDestroyBuffer(Vulkan* vk, VulkanBuffer* buffer) {
	SDL_assert(!buffer->mapped_memory);
	SDL_assert(buffer->handle);
	SDL_assert(buffer->memory);

	vkDestroyBuffer(vk->device, buffer->handle, NULL);
	vkFreeMemory(vk->device, buffer->memory, NULL);

	*buffer = (VulkanBuffer){0};
}

function void VulkanMapBufferMemory(Vulkan* vk, VulkanBuffer* buffer) {
	SDL_assert(!buffer->mapped_memory);
	VK_CHECK(vkMapMemory(vk->device, buffer->memory, buffer->write_offset, buffer->size, 0, &buffer->mapped_memory));
}

function void VulkanUnmapBufferMemory(Vulkan* vk, VulkanBuffer* buffer) {
	SDL_assert(buffer->mapped_memory);
	vkUnmapMemory(vk->device, buffer->memory);
	buffer->mapped_memory = NULL;
	buffer->write_offset = 0;
}

function void VulkanCopyBuffer(VkDeviceSize src_size, void* src, VulkanBuffer* buffer) {
	SDL_assert(buffer->mapped_memory);
	SDL_assert(buffer->write_offset + src_size <= buffer->size);

	uint8_t* mapped_memory = buffer->mapped_memory;
	SDL_memcpy(mapped_memory + buffer->write_offset, src, src_size);
	buffer->write_offset += src_size;
}

function void VulkanCmdCopyBuffer(VkCommandBuffer cb, VulkanBuffer* src, VulkanBuffer* dst, VkDeviceSize size) {
	VkBufferCopy region = {
		.srcOffset = src->read_offset,
		.dstOffset = dst->write_offset,
		.size = SDL_min(size, SDL_min(src->size - src->read_offset, dst->size - dst->write_offset)),
	};
	SDL_assert(region.size != 0);
	vkCmdCopyBuffer(cb, src->handle, dst->handle, 1, &region);
	src->read_offset += region.size;
	dst->write_offset += region.size;
}

function void VulkanResetBuffer(VulkanBuffer* buffer) {
	buffer->read_offset = 0;
	buffer->write_offset = 0;
}

function void VulkanSetImageName(VkDevice device, VkImage image, char* name) {
#ifdef _DEBUG
	{
		VkDebugUtilsObjectNameInfoEXT info = {
			.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT,
			.objectType = VK_OBJECT_TYPE_IMAGE,
			.objectHandle  = (uint64_t)image,
			.pObjectName = name,
		};
		VK_CHECK(vkSetDebugUtilsObjectNameEXT(device, &info));
	}
#else
	UNUSED(device);
	UNUSED(image);
	UNUSED(name);
#endif
}

function void VulkanSetImageViewName(VkDevice device, VkImageView image_view, char* name) {
#ifdef _DEBUG
	{
		VkDebugUtilsObjectNameInfoEXT info = {
			.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT,
			.objectType = VK_OBJECT_TYPE_IMAGE_VIEW,
			.objectHandle  = (uint64_t)image_view,
			.pObjectName = name,
		};
		VK_CHECK(vkSetDebugUtilsObjectNameEXT(device, &info));
	}
#else
	UNUSED(device);
	UNUSED(image_view);
	UNUSED(name);
#endif
}

function void VulkanSetBufferName(VkDevice device, VkBuffer buffer, char* name) {
#ifdef _DEBUG
	{
		VkDebugUtilsObjectNameInfoEXT info = {
			.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT,
			.objectType = VK_OBJECT_TYPE_BUFFER,
			.objectHandle  = (uint64_t)buffer,
			.pObjectName = name,
		};
		VK_CHECK(vkSetDebugUtilsObjectNameEXT(device, &info));
	}
#else
	UNUSED(device);
	UNUSED(buffer);
	UNUSED(name);
#endif
}
