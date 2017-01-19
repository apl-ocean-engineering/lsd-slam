from conans import ConanFile, CMake

class LsdSlamConan(ConanFile):
  name = "lsd_slam"
  version = "master"
  license = 'GPLv3'
  url = 'https://github.com/amarburg/lsd_slam'
  settings = "os", "compiler", "build_type", "arch"
  generators = "cmake"
  options = {"opencv_dir": "ANY",  "build_parallel": [True, False]}
  default_options = "opencv_dir=''", "build_parallel=True"
  exports = ['lib/*', 'include/**', 'test/**', 'tools/**', 'cmake/*.cmake','CMakeLists.txt', 'Rakefile', 'conanfile.py', '.rb/', 'thirdparty/**']
  requires = "TCLAP/master@jmmut/testing", \
              "g3log/0.1@amarburg/testing", \
              'libactive_object/0.1@amarburg/testing', \
              'g2o/master@amarburg/testing', \
              'pangolin/master@amarburg/testing'

  def config(self):
    self.options['g2o'].build_parallel=self.options.build_parallel

    if self.scope.dev and self.scope.build_tests:
      self.requires( "gtest/1.8.0@lasote/stable" )
      self.options["gtest"].shared = False

  def imports(self):
    self.copy("*.dll", dst="bin", src="bin") # From bin to bin
    self.copy("*.dylib*", dst="bin", src="lib") # From lib to bin

  def build(self):
    cmake = CMake(self.settings)
    cmake_opts = "-DUSE_CONAN=True "

    cmake_opts += "-DOpenCV_DIR=%s " % (self.options.opencv_dir) if self.options.opencv_dir else ""
    cmake_opts += "-DBUILD_UNIT_TESTS=1 " if self.scope.dev and self.scope.build_tests else ""

    build_opts = "-j" if self.options.build_parallel else ""

    print('cmake_opts')
    self.run('cmake "%s" %s %s' % (self.conanfile_directory, cmake.command_line, cmake_opts))
    self.run('make deps')
    self.run('cmake --build . %s -- %s' % (cmake.build_config, build_opts))
    if self.scope.dev and self.scope.build_tests:
      self.run('make unit_test')

  def package(self):
    self.copy("*.h", dst="")
    #if self.options.shared:
    if self.settings.os == "Macos":
        self.copy(pattern="*.dylib", dst="lib", keep_path=False)
    else:
        self.copy(pattern="*.so*", dst="lib", src="lib", keep_path=False)
    #else:
    #    self.copy(pattern="*.a", dst="lib", src="lib", keep_path=False)

  def package_info(self):
      self.cpp_info.libs = ["lsdslam"]