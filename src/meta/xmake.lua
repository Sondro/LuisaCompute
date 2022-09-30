BuildProject({
    projectName = "luisa-compute-meta",
    projectType = "shared"
})
add_files("**.cpp")
add_deps("luisa-compute-core")
