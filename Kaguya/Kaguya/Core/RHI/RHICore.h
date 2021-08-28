#pragma once
#include "RHICommon.h"

class IRHIObject;
class IRHIDevice;
class IRHIDeviceChild;
class IRHIResource;
class IRHIDescriptorTable;

class IRHIObject
{
public:
	IRHIObject()		  = default;
	virtual ~IRHIObject() = default;

	unsigned long AddRef();

	unsigned long Release();

	template<typename T>
	T* As()
	{
		return static_cast<T*>(this);
	}

private:
	std::atomic<unsigned long> NumReferences = 1;
};

class IRHIDeviceChild : public IRHIObject
{
public:
	IRHIDeviceChild() noexcept
		: Parent(nullptr)
	{
	}
	IRHIDeviceChild(IRHIDevice* Parent) noexcept
		: Parent(Parent)
	{
	}

	auto GetParentDevice() const noexcept -> IRHIDevice* { return Parent; }

	void SetParentDevice(IRHIDevice* Parent) noexcept
	{
		assert(this->Parent == nullptr);
		this->Parent = Parent;
	}

protected:
	IRHIDevice* Parent;
};

class IRHIRenderPass : public IRHIDeviceChild
{
public:
	using IRHIDeviceChild::IRHIDeviceChild;
};

class IRHIDescriptorTable : public IRHIDeviceChild
{
public:
	using IRHIDeviceChild::IRHIDeviceChild;
};

class IRHIRootSignature : public IRHIDeviceChild
{
public:
	using IRHIDeviceChild::IRHIDeviceChild;
};

class IRHIResource : public IRHIDeviceChild
{
public:
	using IRHIDeviceChild::IRHIDeviceChild;
};

class IRHIBuffer : public IRHIResource
{
public:
	using IRHIResource::IRHIResource;
};

class IRHITexture : public IRHIResource
{
public:
	using IRHIResource::IRHIResource;
};

class IRHIDevice : public IRHIObject
{
public:
	virtual [[nodiscard]] RefPtr<IRHIRenderPass>	  CreateRenderPass(const RenderPassDesc& Desc)			 = 0;
	virtual [[nodiscard]] RefPtr<IRHIDescriptorTable> CreateDescriptorTable(const DescriptorTableDesc& Desc) = 0;
	virtual [[nodiscard]] RefPtr<IRHIRootSignature>	  CreateRootSignature(const RootSignatureDesc& Desc)	 = 0;

	virtual [[nodiscard]] RefPtr<IRHIBuffer>  CreateBuffer(const RHIBufferDesc& Desc)	= 0;
	virtual [[nodiscard]] RefPtr<IRHITexture> CreateTexture(const RHITextureDesc& Desc) = 0;
};

class IRHICommandList : public IRHIDeviceChild
{
};
