@echo off


echo ^<?xml version="1.0" encoding="utf-8"?^> > hello_shadows_h.msbuild
echo ^<Project DefaultTargets="Build" ToolsVersion="15.0" xmlns="http://schemas.microsoft.com/developer/msbuild/2003"^> >> hello_shadows_h.msbuild

ucdev_build_file_generator_r.exe --input ..\..\src\app\ --mode h >> hello_shadows_h.msbuild

echo ^</Project^> >> hello_shadows_h.msbuild

echo ^<?xml version="1.0" encoding="utf-8"?^> > hello_shadows_cpp.msbuild
echo ^<Project DefaultTargets="Build" ToolsVersion="15.0" xmlns="http://schemas.microsoft.com/developer/msbuild/2003"^> >> hello_shadows_cpp.msbuild

ucdev_build_file_generator_r.exe --input ..\..\src\app\ --mode cpp >> hello_shadows_cpp.msbuild

echo ^</Project^> >> hello_shadows_cpp.msbuild

echo ^<?xml version="1.0" encoding="utf-8"?^> > hello_shadows_filters.vcxproj.filters

echo ^<Project DefaultTargets="Build" ToolsVersion="15.0" xmlns="http://schemas.microsoft.com/developer/msbuild/2003"^> >> hello_shadows_filters.vcxproj.filters

ucdev_build_file_generator_r.exe --type filters --input ..\..\src\app\ --mode h >> hello_shadows_filters.vcxproj.filters

ucdev_build_file_generator_r.exe --type filters --input ..\..\src\app\ --mode cpp >> hello_shadows_filters.vcxproj.filters

echo ^</Project^> >> hello_shadows_filters.vcxproj.filters





