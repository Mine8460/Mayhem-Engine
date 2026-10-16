workspace "Engine"
	architecture "x64"
	startproject "Mayhem-Editor"

	configurations
	{
		"Debug",
		"Release",
		"Dist"
	}

outputdir = "%{cfg.buildcfg}-%{cfg.system}-%{cfg.architecture}"

IncludeDir = {}
IncludeDir["GLFW"] = "Mayhem/vendor/GLFW/include"
IncludeDir["GLAD"] = "Mayhem/vendor/GLAD/include"
IncludeDir["ImGui"] = "Mayhem/vendor/imgui"
IncludeDir["GLM"] = "Mayhem/vendor/glm"
IncludeDir["stb"] = "Mayhem/vendor/stb_image"
IncludeDir["entt"] = "Mayhem/vendor/entt"
IncludeDir["yaml"] = "Mayhem/vendor/yaml/include"
IncludeDir["ImGuizmo"] = "Mayhem/vendor/imguizmo/src"
IncludeDir["assimp"] = "Mayhem/vendor/assimp/include"
IncludeDir["assimp_config"] = "Mayhem/vendor/assimp/build/include"

LibraryDir = LibraryDir or {}
LibraryDir["assimp_Debug"] = "Mayhem/vendor/assimp/build/lib/Debug"
LibraryDir["assimp_Release"] = "Mayhem/vendor/assimp/build/lib/Release"
LibraryDir["assimp_bin_Debug"] = "Mayhem/vendor/assimp/build/bin/Debug"
LibraryDir["assimp_bin_Release"] = "Mayhem/vendor/assimp/build/bin/Release"

include "Mayhem/vendor/GLFW"
include "Mayhem/vendor/GLAD"
include "Mayhem/vendor/imgui"
include "Mayhem/vendor/yaml"

project "Mayhem"
	location "Mayhem"
	kind "StaticLib"
	language "C++"
	cppdialect "C++20"
	staticruntime "on"

	targetdir ("bin/" .. outputdir .. "/%{prj.name}")
	objdir ("bin-int/" .. outputdir .. "/%{prj.name}")

	pchheader "Enpch.h"
	pchsource "Mayhem/src/Enpch.cpp"

	removefiles { "Mayhem/vendor/assimp/**" }

	files
	{
		"%{prj.name}/src/**.h",
		"%{prj.name}/src/**.cpp",
		"%{prj.name}/vendor/stb_image/**.cpp",
		"%{prj.name}/vendor/stb_image/**.h",
		"%{prj.name}/vendor/glm/glm/**.hpp",
		"%{prj.name}/vendor/glm/glm/**.inl",

		"%{prj.name}/vendor/imguizmo/src/ImGuizmo.h",
		"%{prj.name}/vendor/imguizmo/src/ImGuizmo.cpp",
	}

	includedirs
	{
		"%{prj.name}/src",
		"%{IncludeDir.GLFW}",
		"%{IncludeDir.GLAD}",
		"%{IncludeDir.ImGui}",
		"%{IncludeDir.GLM}",
		"%{IncludeDir.stb}",
		"%{IncludeDir.entt}",
		"%{IncludeDir.yaml}",
		"%{IncludeDir.ImGuizmo}",
		"%{IncludeDir.assimp}",
		"%{IncludeDir.assimp_config}",
	}

	links
	{
		"GLFW",
		"GLAD",
		"ImGui",
		"yaml-cpp",
		"opengl32.lib"
	}

	filter "files:Mayhem/vendor/imguizmo/**.cpp"
		enablepch "Off"

	filter "system:windows"
		systemversion "latest"

		defines
		{
			YAML_CPP_STATIC_DEFINE,
			"ENGINE_PLATFORM_WINDOWS",
			"ENGINE_BUILD_DLL",
			"GLFW_INCLUDE_NONE"
		}

	filter "configurations:Debug"
		libdirs { "%{LibraryDir.assimp_Debug}" }
		links { "assimp-vc145-mtd.lib" }
		defines "ENGINE_DEBUG"
		runtime "Debug"
		symbols "on"

	filter "configurations:Release"
		libdirs { "%{LibraryDir.assimp_Release}" }
		links { "assimp-vc145-mt.lib" }
		defines "ENGINE_RELEASE"
		runtime "Release"
		optimize "on"

	filter "configurations:Dist"
		libdirs { "%{LibraryDir.assimp_Release}" }
		links { "assimp-vc145-mt.lib" }
		defines "ENGINE_DIST"
		runtime "Release"
		optimize "on"

project "Sandbox"
	location "Sandbox"
	kind "ConsoleApp"
	language "C++"
	cppdialect "C++20"
	staticruntime "on"

	targetdir ("bin/" .. outputdir .. "/%{prj.name}")
	objdir ("bin-int/" .. outputdir .. "/%{prj.name}")

	files
	{
		"%{prj.name}/src/**.h",
		"%{prj.name}/src/**.cpp",
	}

	includedirs
	{
		"Mayhem/vendor/spdlog/include",
		"Mayhem/src",
		"Mayhem/vendor",
		"%{IncludeDir.GLM}",
		"%{IncludeDir.entt}",
		"%{IncludeDir.yaml}",
	}

	links
	{
		"Mayhem",
		"yaml-cpp",
	}

	filter "system:windows"
		systemversion "latest"

		defines
		{
			"ENGINE_PLATFORM_WINDOWS",
			"_CRT_NO_SECURE_WARNINGS"
		}

	filter "configurations:Debug"
		defines "ENGINE_DEBUG"
		runtime "Debug"
		symbols "on"

	filter "configurations:Release"
		defines "ENGINE_RELEASE"
		runtime "Release"
		optimize "on"

	filter "configurations:Dist"
		defines "ENGINE_DIST"
		runtime "Release"
		optimize "on"

project "Mayhem-Editor"
	location "Mayhem-Editor"
	kind "ConsoleApp"
	language "C++"
	cppdialect "C++20"
	staticruntime "on"

	targetdir ("bin/" .. outputdir .. "/%{prj.name}")
	objdir ("bin-int/" .. outputdir .. "/%{prj.name}")

	files
	{
		"%{prj.name}/src/**.h",
		"%{prj.name}/src/**.cpp",
	}

	includedirs
	{
		"Mayhem/vendor/spdlog/include",
		"Mayhem/src",
		"Mayhem/vendor",
		"%{IncludeDir.GLM}",
		"%{IncludeDir.entt}",
		"%{IncludeDir.yaml}",
	}

	links
	{
		"Mayhem",
		"yaml-cpp",
	}


	filter "system:windows"
		systemversion "latest"

		defines
		{
			"ENGINE_PLATFORM_WINDOWS",
			"_CRT_NO_SECURE_WARNINGS"
		}

	filter "configurations:Debug"
		defines "ENGINE_DEBUG"
		runtime "Debug"
		symbols "on"

	filter "configurations:Release"
		defines "ENGINE_RELEASE"
		runtime "Release"
		optimize "on"

	filter "configurations:Dist"
		defines "ENGINE_DIST"
		runtime "Release"
		optimize "on"