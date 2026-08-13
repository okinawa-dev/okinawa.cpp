#include "docsgen.hpp"

#include <cstdio>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

namespace fs = std::filesystem;

namespace {

  std::string readFile(const fs::path &p) {
    std::ifstream      in(p, std::ios::binary);
    std::ostringstream ss;
    ss << in.rdbuf();
    return ss.str();
  }

  void writeFile(const fs::path &p, const std::string &data) {
    fs::create_directories(p.parent_path());
    std::ofstream out(p, std::ios::binary);
    out << data;
  }

  // Default title from a path: "reference/items.md" -> "items".
  std::string titleFromPath(const std::string &srcRelPath) {
    std::string stem = fs::path(srcRelPath).stem().string();
    return stem;
  }

  std::string argValue(int argc, char **argv, const std::string &flag,
                       const std::string &fallback) {
    for (int i = 1; i + 1 < argc; i++) {
      if (flag == argv[i])
        return argv[i + 1];
    }
    return fallback;
  }

}  // namespace

int main(int argc, char **argv) {
  std::string inDir   = argValue(argc, argv, "--in", "docs/content");
  std::string outDir  = argValue(argc, argv, "--out", "docs/dist");
  std::string baseUrl = argValue(argc, argv, "--base-url", "");
  std::string templatePath =
      argValue(argc, argv, "--template", "docs/templates/layout.html");
  std::string staticDir = argValue(argc, argv, "--static", "docs/static");

  if (!fs::exists(inDir)) {
    std::fprintf(stderr, "okinawa-docs: input dir not found: %s\n",
                 inDir.c_str());
    return 1;
  }
  if (!fs::exists(templatePath)) {
    std::fprintf(stderr, "okinawa-docs: template not found: %s\n",
                 templatePath.c_str());
    return 1;
  }

  std::string tmpl = readFile(templatePath);

  // First pass: collect every page.
  std::vector<docsgen::Page> pages;
  fs::path                   inRoot(inDir);
  for (fs::recursive_directory_iterator it(inRoot), end; it != end; ++it) {
    if (!it->is_regular_file())
      continue;
    if (it->path().extension() != ".md")
      continue;

    std::string rel     = fs::relative(it->path(), inRoot).generic_string();
    std::string content = readFile(it->path());

    docsgen::FrontMatter fm;
    std::string          body;
    bool                 hasFm = docsgen::parseFrontMatter(content, fm, body);

    docsgen::Page page;
    page.srcRelPath = rel;
    page.outRelPath = docsgen::outputPathFor(rel);
    page.title   = (hasFm && !fm.title.empty()) ? fm.title : titleFromPath(rel);
    page.section = hasFm ? fm.section : "";
    page.navOrder    = (hasFm && fm.hasNavOrder) ? fm.navOrder : 1000;
    page.hasNavOrder = hasFm && fm.hasNavOrder;
    page.body        = hasFm ? body : content;
    pages.push_back(page);
  }

  // Second pass: render each page (see the pipeline in the plan header).
  fs::path outRoot(outDir);

  // Guard against a destructive --out. We are about to remove_all(outDir), so
  // refuse paths that would wipe the repo, the cwd, the filesystem root, or the
  // very inputs we read from.
  fs::path    outNorm = outRoot.lexically_normal();
  std::string outStr  = outNorm.generic_string();
  if (outStr.empty() || outStr == "." || outStr == "/" || outStr == ".." ||
      outNorm == fs::path(inDir).lexically_normal() ||
      outNorm == fs::path(staticDir).lexically_normal()) {
    std::fprintf(stderr,
                 "okinawa-docs: refusing to use output dir '%s' (too broad or "
                 "overlaps inputs)\n",
                 outDir.c_str());
    return 1;
  }

  fs::remove_all(outRoot);
  for (std::size_t i = 0; i < pages.size(); i++) {
    const docsgen::Page &page = pages[i];
    std::string          nav  = docsgen::applyBaseUrl(
        docsgen::buildNav(pages, page.outRelPath), baseUrl);
    std::string content =
        docsgen::applyBaseUrl(docsgen::renderMarkdown(page.body), baseUrl);
    std::string full =
        docsgen::renderTemplate(tmpl, page.title, nav, content, baseUrl);
    writeFile(outRoot / page.outRelPath, full);
  }

  // Copy static assets to out/static.
  if (fs::exists(staticDir)) {
    fs::copy(staticDir, outRoot / "static",
             fs::copy_options::recursive |
                 fs::copy_options::overwrite_existing);
  }

  std::printf("okinawa-docs: wrote %zu page(s) to %s\n", pages.size(),
              outDir.c_str());
  return 0;
}
