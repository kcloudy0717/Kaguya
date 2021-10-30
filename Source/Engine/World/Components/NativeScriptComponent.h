#pragma once

class ScriptableEntity;

class NativeScriptComponent
{
public:
	template<typename T, typename... TArgs>
	void Bind(TArgs&&... Args)
	{
		static_assert(std::is_base_of_v<ScriptableEntity, T>);
		InstantiateScript = [... Args = std::forward<TArgs>(Args)]() mutable
		{
			return std::make_unique<T>(std::forward<TArgs>(Args)...);
		};
	}

	std::unique_ptr<ScriptableEntity> Instance;

	std::function<std::unique_ptr<ScriptableEntity>()> InstantiateScript;
};
