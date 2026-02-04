#include "Texture.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

namespace Core
{
	Texture::~Texture()
	{
		auto& app = Core::Application::Get();

		vkDestroyImageView(app.GetVulkanDevice(), m_Image.View, nullptr);
		vmaDestroyImage(app.GetVmaAllocator(), m_Image.Image, m_Image.Allocation);
		vkDestroySampler(app.GetVulkanDevice(), m_Sampler, nullptr);
	}

	void Texture::LoadFromFile(const std::filesystem::path& path)
	{
		m_Channels = 4;
		std::string pathStr = path.string();
		int x = 0, y = 0;
		unsigned char* imageData = stbi_load(pathStr.c_str(), &x, &y, 0, m_Channels);

		if (imageData == nullptr)
			return;

		m_Width = static_cast<u32>(x);
		m_Height = static_cast<u32>(y);

		size_t imageSize = m_Width * m_Height * m_Channels;

		auto& app = Core::Application::Get();

		m_Image = app.CreateImage(m_Width, m_Height, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_TILING_OPTIMAL,
			VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT, VMA_MEMORY_USAGE_GPU_ONLY, VK_SAMPLE_COUNT_1_BIT);

		app.TransitionImageLayout(m_Image.Image, m_Image.Format, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

		VkSamplerCreateInfo samplerInfo = {};
		samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
		samplerInfo.magFilter = VK_FILTER_LINEAR;
		samplerInfo.minFilter = VK_FILTER_LINEAR;
		samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
		samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		samplerInfo.minLod = -1000.0f;
		samplerInfo.maxLod = 1000.0f;
		samplerInfo.maxAnisotropy = 1.0f;

		vkCreateSampler(app.GetVulkanDevice(), &samplerInfo, nullptr, &m_Sampler);

		Core::Buffer uploadBuffer = app.CreateBuffer(imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);

		void* data;
		vmaMapMemory(app.GetVmaAllocator(), uploadBuffer.Allocation, &data);
		memcpy(data, imageData, imageSize);
		vmaUnmapMemory(app.GetVmaAllocator(), uploadBuffer.Allocation);

		stbi_image_free(imageData);

		app.CopyBufferToImage(uploadBuffer.Buffer, m_Image.Image, m_Width, m_Height);
		
		app.TransitionImageLayout(m_Image.Image, m_Image.Format, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

		vmaDestroyBuffer(app.GetVmaAllocator(), uploadBuffer.Buffer, uploadBuffer.Allocation);
	}
}