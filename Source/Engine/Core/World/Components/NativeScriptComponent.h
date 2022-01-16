#pragma once

class ScriptableActor;

class NativeScriptComponent
{
public:
	template<typename T, typename... TArgs>
	void Bind(TArgs&&... Args)
	{
		static_assert(std::is_base_of_v<ScriptableActor, T>);
		InstantiateScript = [... Args = std::forward<TArgs>(Args)]() mutable
		{
			return std::make_unique<T>(std::forward<TArgs>(Args)...);
		};
	}

	std::unique_ptr<ScriptableActor> Instance;

	std::function<std::unique_ptr<ScriptableActor>()> InstantiateScript;
};
