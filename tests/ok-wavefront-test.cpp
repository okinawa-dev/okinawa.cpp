#include "okinawa/importers/wavefront.hpp"
#include <catch2/catch_test_macros.hpp>
#include <cstdio>
#include <fstream>
#include <string>
#include <vector>

// Reading a Wavefront file's geometry.
//
// The format writes a face corner as `v`, `v/vt`, `v//vn` or `v/vt/vn`,
// and which one it is depends on what wrote the file. Read as a bare
// integer -- which is what this did -- the first slash stops the stream
// and a face comes out with a single corner, so a model exported with
// texture coordinates loaded as no triangles at all and said nothing.

namespace {

  // A quad written the four ways the format allows, one per file, so a
  // test failure names the form that broke.
  std::string writeTemp(const std::string &name, const std::string &body) {
    std::string path = std::string("/tmp/okinawa-wavefront-") + name + ".obj";
    std::ofstream out(path.c_str());
    out << "v 0 0 0\nv 1 0 0\nv 1 1 0\nv 0 1 0\n";
    out << "vt 0 0\nvt 1 0\nvt 1 1\nvt 0 1\n";
    out << "vn 0 0 1\n";
    out << body;
    out.close();
    return path;
  }

}  // namespace

TEST_CASE("A face is read whatever form its corners are written in",
          "[wavefront]") {
  struct Form {
    const char *name;
    const char *body;
  };
  // Two triangles over the same four vertices, in each of the forms.
  std::vector<Form> forms = {
      {"bare", "f 1 2 3\nf 1 3 4\n"},
      {"uv", "f 1/1 2/2 3/3\nf 1/1 3/3 4/4\n"},
      {"normal", "f 1//1 2//1 3//1\nf 1//1 3//1 4//1\n"},
      {"both", "f 1/1/1 2/2/1 3/3/1\nf 1/1/1 3/3/1 4/4/1\n"}};

  for (size_t i = 0; i < forms.size(); i++) {
    std::string               path = writeTemp(forms[i].name, forms[i].body);
    std::vector<float>        verts;
    std::vector<unsigned int> idx;
    INFO("face form: " << forms[i].name);
    REQUIRE(OkWavefrontImporter::parseGeometry(path, verts, idx));
    REQUIRE(verts.size() == 12);  // four vertices, three floats each
    REQUIRE(idx.size() == 6);     // two triangles
    std::remove(path.c_str());
  }
}

TEST_CASE("A polygon of more than three corners is triangulated",
          "[wavefront]") {
  std::string               path = writeTemp("quad", "f 1/1 2/2 3/3 4/4\n");
  std::vector<float>        verts;
  std::vector<unsigned int> idx;
  REQUIRE(OkWavefrontImporter::parseGeometry(path, verts, idx));
  REQUIRE(idx.size() == 6);  // one quad becomes two triangles
  std::remove(path.c_str());
}

TEST_CASE("A file that is not there is refused rather than assumed empty",
          "[wavefront]") {
  std::vector<float>        verts;
  std::vector<unsigned int> idx;
  REQUIRE_FALSE(OkWavefrontImporter::parseGeometry(
      "/tmp/okinawa-wavefront-does-not-exist.obj", verts, idx));
  REQUIRE(idx.empty());
}
