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
    std::string   path = std::string("/tmp/okinawa-wavefront-") + name + ".obj";
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

// Texture coordinates, normals, and the corner forms that used to be
// read by a second, stricter parser.
//
// `parseMesh` had its own idea of what a face corner looks like: it
// insisted on a slash, so `f 1 2 3` gave no faces at all, and it handed
// the text after the first slash to std::stoi -- which on the perfectly
// legal `f 1//1` is an uncaught exception, an application that stops
// rather than a model that fails to load.

namespace {

  std::string writeMesh(const std::string &name, const std::string &body) {
    std::string   path = std::string("/tmp/okinawa-wavefront-") + name + ".obj";
    std::ofstream out(path.c_str());
    out << body;
    out.close();
    return path;
  }

  // A quad's worth of positions and texture coordinates, and one normal
  // pointing at the viewer.
  const char *const QUAD_HEAD = "v 0 0 0\nv 1 0 0\nv 1 1 0\nv 0 1 0\n"
                                "vt 0 0\nvt 1 0\nvt 1 1\nvt 0 1\n"
                                "vn 0 0 1\n";

}  // namespace

TEST_CASE("A textured face is read whatever form its corners are written in",
          "[wavefront]") {
  struct Form {
    const char *name;
    const char *body;
  };
  std::vector<Form> forms = {{"m-bare", "f 1 2 3\n"},
                             {"m-uv", "f 1/1 2/2 3/3\n"},
                             {"m-normal", "f 1//1 2//1 3//1\n"},
                             {"m-both", "f 1/1/1 2/2/1 3/3/1\n"}};

  for (size_t i = 0; i < forms.size(); i++) {
    std::string path =
        writeMesh(forms[i].name, std::string(QUAD_HEAD) + forms[i].body);
    std::vector<float>        verts;
    std::vector<unsigned int> idx;
    INFO("face form: " << forms[i].name);
    REQUIRE(OkWavefrontImporter::parseMesh(path, verts, idx));
    REQUIRE(idx.size() == 3);
    REQUIRE(verts.size() == 15);  // three corners, five floats each
    std::remove(path.c_str());
  }
}

TEST_CASE("A corner may count backwards from the end", "[wavefront]") {
  // Negative indices are the format's way of referring to the geometry
  // just written, which is what an exporter emitting one object at a
  // time produces.
  std::string path =
      writeMesh("negative", std::string(QUAD_HEAD) + "f -4/-4 -3/-3 -2/-2\n");
  std::vector<float>        verts;
  std::vector<unsigned int> idx;
  REQUIRE(OkWavefrontImporter::parseMesh(path, verts, idx));
  REQUIRE(idx.size() == 3);
  // The same three corners the positive form names.
  REQUIRE(verts[0] == 0.0f);
  REQUIRE(verts[5] == 1.0f);
  std::remove(path.c_str());
}

TEST_CASE("An index past the end is dropped rather than read off the end",
          "[wavefront]") {
  std::string path =
      writeMesh("outofrange", std::string(QUAD_HEAD) + "f 1/1 2/2 99/99\n");
  std::vector<float>        verts;
  std::vector<unsigned int> idx;
  // The face names a corner that is not there, so it is not a triangle
  // and no triangle comes out of it. What must not happen is a read past
  // the end of the vector, which is what the old reader did.
  OkWavefrontImporter::parseMesh(path, verts, idx);
  REQUIRE(idx.empty());
  std::remove(path.c_str());
}

TEST_CASE("The normals a file carries are read, and their absence is said",
          "[wavefront]") {
  std::string withNormals =
      writeMesh("normals", std::string(QUAD_HEAD) + "f 1/1/1 2/2/1 3/3/1\n");
  std::vector<float>        verts;
  std::vector<unsigned int> idx;
  REQUIRE(OkWavefrontImporter::parseMeshWithNormals(withNormals, verts, idx));
  REQUIRE(verts.size() == 24);  // three corners, eight floats each
  // x, y, z, u, v, nx, ny, nz: the normal of the first corner.
  REQUIRE(verts[5] == 0.0f);
  REQUIRE(verts[6] == 0.0f);
  REQUIRE(verts[7] == 1.0f);
  std::remove(withNormals.c_str());

  // A file with no `vn` has nothing this call can add, and says so
  // rather than handing back zeros that would light the model black.
  std::string plain = writeMesh("nonormals", "v 0 0 0\nv 1 0 0\nv 1 1 0\n"
                                             "vt 0 0\nvt 1 0\nvt 1 1\n"
                                             "f 1/1 2/2 3/3\n");
  std::vector<float>        plainVerts;
  std::vector<unsigned int> plainIdx;
  REQUIRE_FALSE(
      OkWavefrontImporter::parseMeshWithNormals(plain, plainVerts, plainIdx));
  std::remove(plain.c_str());
}

TEST_CASE("A face of more than three corners is triangulated with its UVs",
          "[wavefront]") {
  std::string path =
      writeMesh("mquad", std::string(QUAD_HEAD) + "f 1/1 2/2 3/3 4/4\n");
  std::vector<float>        verts;
  std::vector<unsigned int> idx;
  REQUIRE(OkWavefrontImporter::parseMesh(path, verts, idx));
  REQUIRE(idx.size() == 6);
  std::remove(path.c_str());
}
