#pragma once

#include <filesystem>

#include <vulkan/vulkan.h>

#include "Component.h"
#include "Types.h"
#include "Application.h"


namespace Core
{
	class Texture : public Asset
	{
	public:
		~Texture();

		void LoadFromFile(const std::filesystem::path& path);

		[[nodiscard]] VkDescriptorSet GetDescriptorSet() const noexcept { return m_DescriptorSet; }
		[[nodiscard]] u32 GetWidth() const noexcept { return m_Width; }
		[[nodiscard]] u32 GetHeight() const noexcept { return m_Height; }
		[[nodiscard]] u32 GetChannels() const noexcept { return m_Channels; }

		[[nodiscard]] Image& GetImage() noexcept { return m_Image; }
		[[nodiscard]] const Image& GetImage() const noexcept { return m_Image; }
		[[nodiscard]] VkSampler GetSampler() const noexcept { return m_Sampler; }

		void SetDescriptorSet(VkDescriptorSet descriptorSet) noexcept { m_DescriptorSet = descriptorSet; }
 	private:
		VkDescriptorSet m_DescriptorSet;
		u32 m_Width;
		u32 m_Height;
		u32 m_Channels;

		Image m_Image;
		VkSampler m_Sampler;
	};
}