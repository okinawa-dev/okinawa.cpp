from conan import ConanFile
from conan.tools.cmake import CMakeToolchain, CMake, cmake_layout, CMakeDeps

class OkinawaConan(ConanFile):
    name = "okinawa"
    version = "0.1.0"
    
    settings = "os", "compiler", "build_type", "arch"
    options = {"shared": [True, False], "fPIC": [True, False]}
    default_options = {"shared": False, "fPIC": True}
    
    exports_sources = "CMakeLists.txt", "src/*"
    
    def requirements(self):
        self.requires("glm/0.9.9.8")
        self.requires("glfw/3.4")
        self.requires("stb/cci.20240531")
        self.requires("opengl/system")
        
    def build_requirements(self):
        self.test_requires("catch2/3.8.0")
    
    def layout(self):
        cmake_layout(self)
        
    def generate(self):
        deps = CMakeDeps(self)
        deps.generate()
        tc = CMakeToolchain(self)
        tc.generate()
        
    def build(self):
        cmake = CMake(self)
        cmake.configure()
        cmake.build()
        
    def package(self):
        cmake = CMake(self)
        cmake.install()
        
    def package_info(self):
        self.cpp_info.libs = ["okinawa_lib"]
        self.cpp_info.includedirs = ["include"]
